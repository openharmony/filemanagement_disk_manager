/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <functional>
#include <mutex>
#include <tuple>
#include <set>

#include "volumemanager_n_exporter.h"

#include "disk.h"
#include "disk_manager_client.h"
#include "disk_manager_napi_errno.h"
#include "disk_manager_napi_utils.h"
#include "ipc_caller_auth.h"
#include "n_async/n_async_work_callback.h"
#include "n_async/n_async_work_promise.h"
#include "n_func_arg.h"
#include "partition_types.h"
#include "volume_core.h"

#include <sstream>
using namespace OHOS::FileManagement::LibN;

namespace OHOS {
namespace DiskManager {
namespace ModuleVolumeManager {
const std::string FEATURE_STR = "VolumeManager.";

namespace {
constexpr int32_t EXTERNAL_VOL_TYPE = static_cast<int32_t>(VolumeType::EXTERNAL);

bool AppendWantLineUtf8(std::ostringstream &oss, const char *key, napi_env env, napi_value want)
{
    bool has = false;
    FILEMGMT_CALL_BASE(napi_has_named_property(env, want, key, &has), false);
    if (!has) {
        return false;
    }
    napi_value prop = nullptr;
    FILEMGMT_CALL_BASE(napi_get_named_property(env, want, key, &prop), false);
    bool succ = false;
    std::unique_ptr<char[]> buf;
    std::tie(succ, buf, std::ignore) = NVal(env, prop).ToUTF8String();
    if (!succ) {
        return false;
    }
    oss << key << '=' << buf.get() << '\n';
    return true;
}

bool AppendWantLineBool(std::ostringstream &oss, const char *key, napi_env env, napi_value want)
{
    bool has = false;
    FILEMGMT_CALL_BASE(napi_has_named_property(env, want, key, &has), false);
    if (!has) {
        return false;
    }
    napi_value prop = nullptr;
    FILEMGMT_CALL_BASE(napi_get_named_property(env, want, key, &prop), false);
    bool bv = false;
    FILEMGMT_CALL_BASE(napi_get_value_bool(env, prop, &bv), false);
    oss << key << '=' << (bv ? "true" : "false") << '\n';
    return true;
}

bool AppendWantLineInt32(std::ostringstream &oss, const char *key, napi_env env, napi_value want)
{
    bool has = false;
    FILEMGMT_CALL_BASE(napi_has_named_property(env, want, key, &has), false);
    if (!has) {
        return false;
    }
    napi_value prop = nullptr;
    FILEMGMT_CALL_BASE(napi_get_named_property(env, want, key, &prop), false);
    bool succ = false;
    int32_t iv = 0;
    std::tie(succ, iv) = NVal(env, prop).ToInt32();
    if (!succ) {
        return false;
    }
    oss << key << '=' << iv << '\n';
    return true;
}

std::string BuildBurnOptionsLines(napi_env env, napi_value wantObj)
{
    std::ostringstream oss;
    (void)AppendWantLineUtf8(oss, "diskName", env, wantObj);
    (void)AppendWantLineUtf8(oss, "burnPath", env, wantObj);
    (void)AppendWantLineBool(oss, "isIsoImage", env, wantObj);
    (void)AppendWantLineInt32(oss, "burnSpeed", env, wantObj);
    (void)AppendWantLineUtf8(oss, "fsType", env, wantObj);
    (void)AppendWantLineBool(oss, "isIncBurnSupport", env, wantObj);
    (void)AppendWantLineBool(oss, "isVerifyBurn", env, wantObj);
    return oss.str();
}

void FillVolumeJSObject(napi_env env, NVal &volObj, const VolumeExternal &v)
{
    volObj.AddProp("id", NVal::CreateUTF8String(env, v.GetId()).val_);
    volObj.AddProp("uuid", NVal::CreateUTF8String(env, v.GetUuid()).val_);
    volObj.AddProp("diskId", NVal::CreateUTF8String(env, v.GetDiskId()).val_);
    volObj.AddProp("description", NVal::CreateUTF8String(env, v.GetDescription()).val_);
    volObj.AddProp("removable", NVal::CreateBool(env, v.GetType() == EXTERNAL_VOL_TYPE).val_);
    volObj.AddProp("state", NVal::CreateInt32(env, v.GetState()).val_);
    volObj.AddProp("path", NVal::CreateUTF8String(env, v.GetPath()).val_);
    volObj.AddProp("fsType", NVal::CreateUTF8String(env, v.GetFsTypeString()).val_);
    volObj.AddProp("partitionNum", NVal::CreateInt32(env, v.GetPartitionNum()).val_);
    const std::string &extra = v.GetExtraInfo();
    if (!extra.empty()) {
        volObj.AddProp("extraInfo", NVal::CreateUTF8String(env, extra).val_);
    }
}

napi_value BuildDiskJSObject(napi_env env, const Disk &d)
{
    NVal diskObj = NVal::CreateObject(env);
    diskObj.AddProp("diskId", NVal::CreateUTF8String(env, d.GetDiskId()).val_);
    napi_value sizeBytesVal = nullptr;
    FILEMGMT_CALL_BASE(napi_create_int64(env, d.GetSizeBytes(), &sizeBytesVal), nullptr);
    diskObj.AddProp("sizeBytes", sizeBytesVal);
    diskObj.AddProp("diskType", NVal::CreateInt32(env, d.GetDiskType()).val_);
    diskObj.AddProp("removable", NVal::CreateBool(env, d.GetRemovable()).val_);
    const std::vector<std::string> &volumeIds = d.GetVolumeIds();
    napi_value volIdArr = nullptr;
    FILEMGMT_CALL_BASE(napi_create_array(env, &volIdArr), nullptr);
    for (size_t i = 0; i < volumeIds.size(); ++i) {
        napi_value vid = NVal::CreateUTF8String(env, volumeIds[i]).val_;
        FILEMGMT_CALL_BASE(napi_set_element(env, volIdArr, static_cast<uint32_t>(i), vid), nullptr);
    }
    diskObj.AddProp("volumeIds", volIdArr);
    const std::string &extra = d.GetExtraInfo();
    if (!extra.empty()) {
        diskObj.AddProp("extraInfo", NVal::CreateUTF8String(env, extra).val_);
    }
    return diskObj.val_;
}

/** 将磁盘列表编成 JS 数组；用于降低 GetAllDisks cbComplete 的嵌套深度（静态扫描阈值）。 */
NVal MakeDiskJsArrayFromVector(napi_env env, const std::vector<Disk> &disks)
{
    napi_value arr = nullptr;
    napi_status status = napi_create_array(env, &arr);
    if (status != napi_ok) {
        return {env, NError(status).GetNapiErr(env)};
    }
    for (size_t i = 0; i < disks.size(); ++i) {
        napi_value dj = BuildDiskJSObject(env, disks[i]);
        status = napi_set_element(env, arr, static_cast<uint32_t>(i), dj);
        if (status != napi_ok) {
            return {env, NError(status).GetNapiErr(env)};
        }
    }
    return {NVal(env, arr)};
}

napi_value ScheduleVolumeGetOpProcess(napi_env env, const std::string &volumeIdStr, NVal &thisVar, NFuncArg &funcArg)
{
    auto progress = std::make_shared<int32_t>(0);
    auto cbExec = [volumeIdStr, progress]() -> NError {
        int32_t errNum = OHOS::DiskManager::DiskManagerClient::GetInstance().GetVolumeOpProcess(
            volumeIdStr, *progress);
        if (errNum != E_OK) {
            return NError(Convert2JsErrNum(errNum));
        }
        return NError(ERRNO_NOERR);
    };
    auto cbComplete = [progress](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return {NVal::CreateInt32(env, *progress)};
    };
    constexpr const char *kProcName = "getOpProcess";
    if (funcArg.GetArgc() == (uint)NARG_CNT::ONE) {
        return NAsyncWorkPromise(env, thisVar).Schedule(kProcName, cbExec, cbComplete).val_;
    }
    NVal cb(env, funcArg[(int)NARG_POS::SECOND]);
    return NAsyncWorkCallback(env, thisVar, cb, FEATURE_STR + __FUNCTION__)
        .Schedule(kProcName, cbExec, cbComplete)
        .val_;
}

} // namespace

// ========== 枚举导出函数 ==========

/**
 * 创建 DiskType 枚举对象
 */
napi_value CreateDiskTypeEnum(napi_env env)
{
    napi_value diskTypeObj = nullptr;
    napi_create_object(env, &diskTypeObj);

    // 使用 disk.h 中定义的 DiskType 枚举值
    napi_property_descriptor props[] = {
        {"SD_CARD", nullptr, nullptr, nullptr, nullptr,
         NVal::CreateInt32(env, static_cast<int32_t>(DiskType::SD_FLAG)).val_, napi_enumerable},
        {"USB_FLASH", nullptr, nullptr, nullptr, nullptr,
         NVal::CreateInt32(env, static_cast<int32_t>(DiskType::USB_FLAG)).val_, napi_enumerable},
        {"CD_DVD_BD", nullptr, nullptr, nullptr, nullptr,
         NVal::CreateInt32(env, static_cast<int32_t>(DiskType::CD_FLAG)).val_, napi_enumerable},
        {"DATA_DISK_SSD", nullptr, nullptr, nullptr, nullptr,
         NVal::CreateInt32(env, static_cast<int32_t>(DiskType::DATA_DISK_SSD)).val_, napi_enumerable},
        {"DATA_DISK_HDD", nullptr, nullptr, nullptr, nullptr,
         NVal::CreateInt32(env, static_cast<int32_t>(DiskType::DATA_DISK_HDD)).val_, napi_enumerable},
        {"DVR_USB", nullptr, nullptr, nullptr, nullptr,
         NVal::CreateInt32(env, static_cast<int32_t>(DiskType::DVR_USB)).val_, napi_enumerable},
        {"UNKNOWN_DISK_TYPE", nullptr, nullptr, nullptr, nullptr,
         NVal::CreateInt32(env, static_cast<int32_t>(DiskType::DISK_TYPE_UNKNOWN)).val_, napi_enumerable},
    };
    napi_define_properties(env, diskTypeObj, sizeof(props) / sizeof(props[0]), props);

    return diskTypeObj;
}

/**
 * 创建 VerifyType 枚举对象
 */
napi_value CreateVerifyTypeEnum(napi_env env)
{
    napi_value verifyTypeObj = nullptr;
    napi_create_object(env, &verifyTypeObj);

    napi_property_descriptor props[] = {
        {"KEY_DATA", nullptr, nullptr, nullptr, nullptr, NVal::CreateInt32(env, 0).val_, napi_enumerable},
        {"FULL_DATA", nullptr, nullptr, nullptr, nullptr, NVal::CreateInt32(env, 1).val_, napi_enumerable},
    };
    napi_define_properties(env, verifyTypeObj, sizeof(props) / sizeof(props[0]), props);

    return verifyTypeObj;
}

// ========== 原有函数实现 ==========

bool CheckVolumes(napi_env env, napi_callback_info info, NFuncArg &funcArg)
{
    if (!IsSystemApp()) {
        NError(E_PERMISSION_SYS).ThrowErr(env);
        return false;
    }
    if (!funcArg.InitArgs((int)NARG_CNT::ZERO, (int)NARG_CNT::ONE)) {
        NError(E_PARAMS).ThrowErr(env);
        return false;
    }
    return true;
}

// 辅助函数：构建 VolumeExternal 向量对应的 JS 数组
static NVal BuildVolumeArrayFromVector(napi_env env, const std::vector<VolumeExternal> &volumes)
{
    napi_value volumeInfoArray = nullptr;
    napi_status status = napi_create_array(env, &volumeInfoArray);
    if (status != napi_ok) {
        return {env, NError(status).GetNapiErr(env)};
    }
    for (size_t i = 0; i < volumes.size(); i++) {
        NVal volumeInfoObject = NVal::CreateObject(env);
        FillVolumeJSObject(env, volumeInfoObject, volumes[i]);
        status = napi_set_element(env, volumeInfoArray, i, volumeInfoObject.val_);
        if (status != napi_ok) {
            return {env, NError(status).GetNapiErr(env)};
        }
    }
    return {NVal(env, volumeInfoArray)};
}

napi_value GetAllVolumes(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!CheckVolumes(env, info, funcArg)) {
        return nullptr;
    }
    auto volumeInfo = std::make_shared<std::vector<VolumeExternal>>();
    auto cbExec = [volumeInfo]() -> NError {
        int32_t errNum =
            OHOS::DiskManager::DiskManagerClient::GetInstance().GetAllVolumes(*volumeInfo);
        if (errNum != E_OK) {
            return NError(Convert2JsErrNum(errNum));
        }
        return NError(ERRNO_NOERR);
    };
    auto cbComplete = [volumeInfo](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return BuildVolumeArrayFromVector(env, *volumeInfo);
    };
    NVal thisVar(env, funcArg.GetThisVar());
    if (funcArg.GetArgc() == (uint)NARG_CNT::ZERO) {
        return NAsyncWorkPromise(env, thisVar).Schedule("GetAllVolumes", cbExec, cbComplete).val_;
    }
    NVal cb(env, funcArg[(int)NARG_POS::FIRST]);
    return NAsyncWorkCallback(env, thisVar, cb, FEATURE_STR + __FUNCTION__)
        .Schedule("GetAllVolumes", cbExec, cbComplete)
        .val_;
}

bool CheckMount(napi_env env, napi_callback_info info, NFuncArg &funcArg)
{
    if (!IsSystemApp()) {
        NError(E_PERMISSION_SYS).ThrowErr(env);
        return false;
    }
    if (!funcArg.InitArgs((int)NARG_CNT::ONE, (int)NARG_CNT::TWO)) {
        NError(E_PARAMS).ThrowErr(env);
        return false;
    }
    return true;
}

napi_value Mount(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    bool checkresult = CheckMount(env, info, funcArg);
    if (!checkresult) {
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> volumeId;
    std::tie(succ, volumeId, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    std::string volumeIdString(volumeId.get());
    auto cbExec = [volumeIdString]() -> NError {
        int32_t result = OHOS::DiskManager::DiskManagerClient::GetInstance().Mount(volumeIdString);
        if (result != E_OK) {
            return NError(Convert2JsErrNum(result));
        }
        return NError(ERRNO_NOERR);
    };
    auto cbComplete = [](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return {NVal::CreateUndefined(env)};
    };

    std::string procedureName = "Mount";
    NVal thisVar(env, funcArg.GetThisVar());
    if (funcArg.GetArgc() == (uint)NARG_CNT::ONE) {
        return NAsyncWorkPromise(env, thisVar).Schedule(procedureName, cbExec, cbComplete).val_;
    } else {
        NVal cb(env, funcArg[(int)NARG_POS::SECOND]);
        return NAsyncWorkCallback(env, thisVar, cb, FEATURE_STR + __FUNCTION__)
            .Schedule(procedureName, cbExec, cbComplete)
            .val_;
    }
}

napi_value Unmount(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    bool checkresult = CheckMount(env, info, funcArg);
    if (!checkresult) {
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> volumeId;
    std::tie(succ, volumeId, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    std::string volumeIdString(volumeId.get());
    auto cbExec = [volumeIdString]() -> NError {
        int32_t result = OHOS::DiskManager::DiskManagerClient::GetInstance().Unmount(volumeIdString);
        if (result != E_OK) {
            return NError(Convert2JsErrNum(result));
        }
        return NError(ERRNO_NOERR);
    };
    auto cbComplete = [](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return {NVal::CreateUndefined(env)};
    };

    std::string procedureName = "Unmount";
    NVal thisVar(env, funcArg.GetThisVar());
    if (funcArg.GetArgc() == (uint)NARG_CNT::ONE) {
        return NAsyncWorkPromise(env, thisVar).Schedule(procedureName, cbExec, cbComplete).val_;
    } else {
        NVal cb(env, funcArg[(int)NARG_POS::SECOND]);
        return NAsyncWorkCallback(env, thisVar, cb, FEATURE_STR + __FUNCTION__)
            .Schedule(procedureName, cbExec, cbComplete)
            .val_;
    }
}

napi_value GetVolumeByUuid(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    bool checkresult = CheckMount(env, info, funcArg);
    if (!checkresult) {
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> uuid;
    std::tie(succ, uuid, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    auto volumeInfo = std::make_shared<VolumeExternal>();
    std::string uuidString(uuid.get());
    auto cbExec = [uuidString, volumeInfo]() -> NError {
        int32_t errNum = OHOS::DiskManager::DiskManagerClient::GetInstance().GetVolumeByUuid(
            uuidString, *volumeInfo);
        if (errNum != E_OK) {
            return NError(Convert2JsErrNum(errNum));
        }
        return NError(ERRNO_NOERR);
    };
    auto cbComplete = [volumeInfo](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        NVal volumeObject = NVal::CreateObject(env);
        FillVolumeJSObject(env, volumeObject, *volumeInfo);
        return volumeObject;
    };

    std::string procedureName = "GetVolumeByUuid";
    NVal thisVar(env, funcArg.GetThisVar());
    if (funcArg.GetArgc() == (uint)NARG_CNT::ONE) {
        return NAsyncWorkPromise(env, thisVar).Schedule(procedureName, cbExec, cbComplete).val_;
    } else {
        NVal cb(env, funcArg[(int)NARG_POS::SECOND]);
        return NAsyncWorkCallback(env, thisVar, cb, FEATURE_STR + __FUNCTION__)
            .Schedule(procedureName, cbExec, cbComplete)
            .val_;
    }
}

napi_value GetVolumeById(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    bool checkresult = CheckMount(env, info, funcArg);
    if (!checkresult) {
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> volumeId;
    std::tie(succ, volumeId, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    auto volumeInfo = std::make_shared<VolumeExternal>();
    std::string volumeIdString(volumeId.get());
    auto cbExec = [volumeIdString, volumeInfo]() -> NError {
        int32_t errNum = OHOS::DiskManager::DiskManagerClient::GetInstance().GetVolumeById(
            volumeIdString, *volumeInfo);
        if (errNum != E_OK) {
            return NError(Convert2JsErrNum(errNum));
        }
        return NError(ERRNO_NOERR);
    };
    auto cbComplete = [volumeInfo](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        NVal volumeObject = NVal::CreateObject(env);
        FillVolumeJSObject(env, volumeObject, *volumeInfo);

        return volumeObject;
    };

    std::string procedureName = "GetVolumeById";
    NVal thisVar(env, funcArg.GetThisVar());
    if (funcArg.GetArgc() == (uint)NARG_CNT::ONE) {
        return NAsyncWorkPromise(env, thisVar).Schedule(procedureName, cbExec, cbComplete).val_;
    } else {
        NVal cb(env, funcArg[(int)NARG_POS::SECOND]);
        return NAsyncWorkCallback(env, thisVar, cb, FEATURE_STR + __FUNCTION__)
            .Schedule(procedureName, cbExec, cbComplete)
            .val_;
    }
}

// 辅助函数：解析两个字符串参数（uuid和description/volumeId等）
static bool ParseTwoStringArgs(napi_env env, NFuncArg &funcArg, std::string &outFirst, std::string &outSecond)
{
    bool succ = false;
    std::unique_ptr<char[]> first;
    std::unique_ptr<char[]> second;
    std::tie(succ, first, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return false;
    }
    std::tie(succ, second, std::ignore) = NVal(env, funcArg[(int)NARG_POS::SECOND]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return false;
    }
    outFirst = std::string(first.get());
    outSecond = std::string(second.get());
    return true;
}

static napi_value PromiseVoidOp(napi_env env,
                                NVal thisVar,
                                const std::string &procName,
                                const std::function<int32_t()> &ipcCall)
{
    auto cbExec = [ipcCall]() -> NError {
        int32_t result = ipcCall();
        if (result != E_OK) {
            return NError(Convert2JsErrNum(result));
        }
        return NError(ERRNO_NOERR);
    };
    auto cbComplete = [](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return {NVal::CreateUndefined(env)};
    };
    return NAsyncWorkPromise(env, thisVar).Schedule(procName, cbExec, cbComplete).val_;
}

napi_value SetVolumeDescription(napi_env env, napi_callback_info info)
{
    if (!IsSystemApp()) {
        NError(E_PERMISSION_SYS).ThrowErr(env);
        return nullptr;
    }
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs((int)NARG_CNT::TWO, (int)NARG_CNT::THREE)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    std::string uuidString;
    std::string descStr;
    if (!ParseTwoStringArgs(env, funcArg, uuidString, descStr)) {
        return nullptr;
    }

    NVal thisVar(env, funcArg.GetThisVar());
    return PromiseVoidOp(env, thisVar, "SetVolumeDescription", [uuidString, descStr]() {
        return OHOS::DiskManager::DiskManagerClient::GetInstance().SetVolumeDescription(
            uuidString, descStr);
    });
}

napi_value Format(napi_env env, napi_callback_info info)
{
    if (!IsSystemApp()) {
        NError(E_PERMISSION_SYS).ThrowErr(env);
        return nullptr;
    }
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs((int)NARG_CNT::TWO, (int)NARG_CNT::THREE)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    std::string volumeIdString;
    std::string fsTypeString;
    if (!ParseTwoStringArgs(env, funcArg, volumeIdString, fsTypeString)) {
        return nullptr;
    }

    NVal thisVar(env, funcArg.GetThisVar());
    return PromiseVoidOp(env, thisVar, "Format", [volumeIdString, fsTypeString]() {
        return OHOS::DiskManager::DiskManagerClient::GetInstance().Format(
            volumeIdString, fsTypeString);
    });
}

// 辅助函数：解析字符串和int32参数
static bool ParseStringAndInt32Args(napi_env env, NFuncArg &funcArg, std::string &outStr, int32_t &outInt)
{
    bool succ = false;
    std::unique_ptr<char[]> str;
    std::tie(succ, str, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return false;
    }
    std::tie(succ, outInt) = NVal(env, funcArg[(int)NARG_POS::SECOND]).ToInt32();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return false;
    }
    outStr = std::string(str.get());
    return true;
}

napi_value Partition(napi_env env, napi_callback_info info)
{
    if (!IsSystemApp()) {
        NError(E_PERMISSION_SYS).ThrowErr(env);
        return nullptr;
    }
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs((int)NARG_CNT::TWO, (int)NARG_CNT::THREE)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    std::string diskIdString;
    int32_t type;
    if (!ParseStringAndInt32Args(env, funcArg, diskIdString, type)) {
        return nullptr;
    }

    NVal thisVar(env, funcArg.GetThisVar());
    return PromiseVoidOp(env, thisVar, "Partition", [diskIdString, type]() {
        return OHOS::DiskManager::DiskManagerClient::GetInstance().Partition(diskIdString, type);
    });
}

napi_value GetAllDisks(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!IsSystemApp()) {
        NError(E_PERMISSION_SYS).ThrowErr(env);
        return nullptr;
    }
    if (!VerifyCallerPermission(PERMISSION_MOUNT_MANAGER)) {
        NError(E_PERMISSION).ThrowErr(env);
        return nullptr;
    }
    if (!funcArg.InitArgs((int)NARG_CNT::ZERO)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    auto disks = std::make_shared<std::vector<Disk>>();
    auto cbExec = [disks]() -> NError {
        int32_t errNum = OHOS::DiskManager::DiskManagerClient::GetInstance().GetAllDisks(*disks);
        if (errNum != E_OK) {
            return NError(Convert2JsErrNum(errNum));
        }
        return NError(ERRNO_NOERR);
    };
    auto cbComplete = [disks](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return MakeDiskJsArrayFromVector(env, *disks);
    };
    NVal thisVar(env, funcArg.GetThisVar());
    return NAsyncWorkPromise(env, thisVar).Schedule("GetAllDisks", cbExec, cbComplete).val_;
}

napi_value GetDiskById(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs((int)NARG_CNT::ONE)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    NVal paramsFirstNVal(env, funcArg[(int)NARG_POS::FIRST]);
    if (paramsFirstNVal.val_ == nullptr) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> diskId;
    std::tie(succ, diskId, std::ignore) = paramsFirstNVal.ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    std::string diskIdStr(diskId.get());
    auto disk = std::make_shared<Disk>();
    auto cbExec = [diskIdStr, disk]() -> NError {
        int32_t errNum =
            DiskManagerClient::GetInstance().GetDiskById(diskIdStr, *disk);
        if (errNum != E_OK) {
            return NError(Convert2JsErrNum(errNum));
        }
        return NError(ERRNO_NOERR);
    };
    auto cbComplete = [disk](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        napi_value dj = BuildDiskJSObject(env, *disk);
        return {NVal(env, dj)};
    };
    NVal thisVar(env, funcArg.GetThisVar());
    return NAsyncWorkPromise(env, thisVar).Schedule("GetDiskById", cbExec, cbComplete).val_;
}

napi_value Erase(napi_env env, napi_callback_info info)
{
    if (!IsSystemApp()) {
        NError(E_PERMISSION_SYS).ThrowErr(env);
        return nullptr;
    }
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs((int)NARG_CNT::ONE, (int)NARG_CNT::TWO)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> volumeId;
    std::tie(succ, volumeId, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    std::string volumeIdStr(volumeId.get());
    NVal thisVar(env, funcArg.GetThisVar());
    return PromiseVoidOp(env, thisVar, "Erase", [volumeIdStr]() {
        return OHOS::DiskManager::DiskManagerClient::GetInstance().Erase(volumeIdStr);
    });
}

napi_value Eject(napi_env env, napi_callback_info info)
{
    if (!IsSystemApp()) {
        NError(E_PERMISSION_SYS).ThrowErr(env);
        return nullptr;
    }
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs((int)NARG_CNT::ONE, (int)NARG_CNT::TWO)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> diskId;
    std::tie(succ, diskId, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    std::string diskIdString(diskId.get());
    NVal thisVar(env, funcArg.GetThisVar());
    return PromiseVoidOp(env, thisVar, "Eject", [diskIdString]() {
        return OHOS::DiskManager::DiskManagerClient::GetInstance().Eject(diskIdString);
    });
}

napi_value CreateIsoImage(napi_env env, napi_callback_info info)
{
    if (!IsSystemApp()) {
        NError(E_PERMISSION_SYS).ThrowErr(env);
        return nullptr;
    }
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs((int)NARG_CNT::TWO, (int)NARG_CNT::THREE)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> volumeId;
    std::unique_ptr<char[]> filePath;
    std::tie(succ, volumeId, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    std::tie(succ, filePath, std::ignore) = NVal(env, funcArg[(int)NARG_POS::SECOND]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    std::string volIdStr(volumeId.get());
    std::string filePathStr(filePath.get());
    NVal thisVar(env, funcArg.GetThisVar());
    return PromiseVoidOp(env, thisVar, "CreateIsoImage", [volIdStr, filePathStr]() {
        return OHOS::DiskManager::DiskManagerClient::GetInstance().CreateIsoImage(volIdStr, filePathStr);
    });
}

napi_value Burn(napi_env env, napi_callback_info info)
{
    if (!IsSystemApp()) {
        NError(E_PERMISSION_SYS).ThrowErr(env);
        return nullptr;
    }
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs((int)NARG_CNT::TWO, (int)NARG_CNT::THREE)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    
    // 解析 volumeId
    bool succ = false;
    std::unique_ptr<char[]> volumeId;
    std::tie(succ, volumeId, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    
    // 获取 want 对象
    napi_valuetype wantType = napi_undefined;
    NVal wantArg(env, funcArg[(int)NARG_POS::SECOND]);
    if (napi_typeof(env, wantArg.val_, &wantType) != napi_ok || wantType != napi_object) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    
    // 提取 parameters 对象
    napi_value parameters = nullptr;
    bool hasParameters = false;
    napi_has_named_property(env, wantArg.val_, "parameters", &hasParameters);
    if (!hasParameters) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    napi_get_named_property(env, wantArg.val_, "parameters", &parameters);
    
    // 使用 parameters 对象构建 burnOpts
    std::string burnOpts = BuildBurnOptionsLines(env, parameters);
    
    std::string volumeIdStr(volumeId.get());
    NVal thisVar(env, funcArg.GetThisVar());
    return PromiseVoidOp(env, thisVar, "Burn", [volumeIdStr, burnOpts]() {
        return OHOS::DiskManager::DiskManagerClient::GetInstance().Burn(volumeIdStr, burnOpts);
    });
}

napi_value GetOpProcess(napi_env env, napi_callback_info info)
{
    if (!IsSystemApp()) {
        NError(E_PERMISSION_SYS).ThrowErr(env);
        return nullptr;
    }
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs((int)NARG_CNT::ONE, (int)NARG_CNT::TWO)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> volumeId;
    std::tie(succ, volumeId, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    NVal thisVar(env, funcArg.GetThisVar());
    return ScheduleVolumeGetOpProcess(env, std::string(volumeId.get()), thisVar, funcArg);
}

napi_value IsVolumeInUse(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs((int)NARG_CNT::ONE)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> volumePath;
    std::tie(succ, volumePath, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    std::string volumePathStr(volumePath.get());
    auto isInUse = std::make_shared<bool>(true);
    auto cbExec = [volumePathStr, isInUse]() -> NError {
        int32_t errNum = OHOS::DiskManager::DiskManagerClient::GetInstance().QueryUsbIsInUse(volumePathStr, *isInUse);
        if (errNum != E_OK) {
            return NError(Convert2JsErrNum(errNum));
        }
        return NError(ERRNO_NOERR);
    };
    auto cbComplete = [isInUse](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return {NVal::CreateBool(env, *isInUse)};
    };
    NVal thisVar(env, funcArg.GetThisVar());
    return NAsyncWorkPromise(env, thisVar).Schedule("IsVolumeInUse", cbExec, cbComplete).val_;
}

// ========== Partition management APIs ==========

// 辅助函数：构建单个分区信息JS对象
static NVal BuildPartitionInfoJSObject(napi_env env, const PartitionInfo &partInfo)
{
    NVal partObj = NVal::CreateObject(env);
    partObj.AddProp("partitionNum", NVal::CreateInt32(env, partInfo.GetPartitionNum()).val_);
    partObj.AddProp("diskId", NVal::CreateUTF8String(env, partInfo.GetDiskId()).val_);

    napi_value startSectorVal = nullptr;
    napi_value endSectorVal = nullptr;
    napi_value sizeBytesVal = nullptr;
    napi_create_int64(env, partInfo.GetStartSector(), &startSectorVal);
    napi_create_int64(env, partInfo.GetEndSector(), &endSectorVal);
    napi_create_int64(env, partInfo.GetSizeBytes(), &sizeBytesVal);
    partObj.AddProp("startSector", startSectorVal);
    partObj.AddProp("endSector", endSectorVal);
    partObj.AddProp("sizeBytes", sizeBytesVal);
    partObj.AddProp("fsType", NVal::CreateUTF8String(env, partInfo.GetFsType()).val_);

    return partObj;
}

// 辅助函数：构建分区表信息JS对象
static NVal BuildPartitionTableInfoJSObject(napi_env env, const PartitionTableInfo &tableInfo)
{
    NVal obj = NVal::CreateObject(env);
    obj.AddProp("diskId", NVal::CreateUTF8String(env, tableInfo.GetDiskId()).val_);
    obj.AddProp("tableType", NVal::CreateUTF8String(env, tableInfo.GetTableType()).val_);
    obj.AddProp("partitionCount", NVal::CreateInt32(env, tableInfo.GetPartitionCount()).val_);

    napi_value totalSectorVal = nullptr;
    napi_create_int64(env, tableInfo.GetTotalSector(), &totalSectorVal);
    obj.AddProp("totalSector", totalSectorVal);
    obj.AddProp("sectorSize", NVal::CreateInt32(env, tableInfo.GetSectorSize()).val_);
    obj.AddProp("alignSector", NVal::CreateInt32(env, tableInfo.GetAlignSector()).val_);

    const auto &partitions = tableInfo.GetPartitions();
    napi_value partitionsArr = nullptr;
    napi_create_array(env, &partitionsArr);
    for (size_t i = 0; i < partitions.size(); ++i) {
        NVal partObj = BuildPartitionInfoJSObject(env, partitions[i]);
        napi_set_element(env, partitionsArr, static_cast<uint32_t>(i), partObj.val_);
    }
    obj.AddProp("partitions", partitionsArr);

    return obj;
}

napi_value GetPartitionTable(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs((int)NARG_CNT::ONE)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    NVal paramsFirstNVal(env, funcArg[(int)NARG_POS::FIRST]);
    if (paramsFirstNVal.val_ == nullptr) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> diskId;
    std::tie(succ, diskId, std::ignore) = paramsFirstNVal.ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    std::string diskIdStr(diskId.get());
    auto tableInfo = std::make_shared<PartitionTableInfo>();
    auto cbExec = [diskIdStr, tableInfo]() -> NError {
        int32_t errNum = DiskManagerClient::GetInstance().GetPartitionTable(diskIdStr, *tableInfo);
        if (errNum != E_OK) {
            return NError(Convert2JsErrNum(errNum));
        }
        return NError(ERRNO_NOERR);
    };
    auto cbComplete = [tableInfo](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return BuildPartitionTableInfoJSObject(env, *tableInfo);
    };
    NVal thisVar(env, funcArg.GetThisVar());
    return NAsyncWorkPromise(env, thisVar).Schedule("GetPartitionTable", cbExec, cbComplete).val_;
}

// 辅助函数：解析 PartitionParams 参数
static bool ParsePartitionParams(napi_env env, napi_value paramsObj, PartitionParams &params)
{
    bool succ = false;
    NVal paramsNVal(env, paramsObj);
    if (paramsNVal.val_ == nullptr) {
        return false;
    }
    NVal partNumVal = paramsNVal.GetProp("partitionNum");
    if (partNumVal.val_ == nullptr) {
        return false;
    }
    int32_t partitionNum = 0;
    std::tie(succ, partitionNum) = partNumVal.ToInt32();
    if (!succ) {
        return false;
    }
    params.SetPartitionNum(partitionNum);
    NVal startSectorVal = paramsNVal.GetProp("startSector");
    if (startSectorVal.val_ == nullptr) {
        return false;
    }
    int64_t startSector = 0;
    std::tie(succ, startSector) = startSectorVal.ToInt64();
    if (!succ) {
        return false;
    }
    params.SetStartSector(startSector);
    NVal endSectorVal = paramsNVal.GetProp("endSector");
    if (endSectorVal.val_ == nullptr) {
        return false;
    }
    int64_t endSector = 0;
    std::tie(succ, endSector) = endSectorVal.ToInt64();
    if (!succ) {
        return false;
    }
    params.SetEndSector(endSector);
    NVal typeCodeVal = paramsNVal.GetProp("typeCode");
    if (typeCodeVal.val_ == nullptr) {
        return false;
    }
    std::unique_ptr<char[]> typeCode;
    std::tie(succ, typeCode, std::ignore) = typeCodeVal.ToUTF8String();
    if (!succ) {
        return false;
    }
    params.SetTypeCode(std::string(typeCode.get()));
    return true;
}

napi_value CreatePartition(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs((int)NARG_CNT::TWO)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    NVal paramsFirstNVal(env, funcArg[(int)NARG_POS::FIRST]);
    if (paramsFirstNVal.val_ == nullptr) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    NVal paramsSecondNVal(env, funcArg[(int)NARG_POS::SECOND]);
    if (paramsSecondNVal.val_ == nullptr) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> diskId;
    std::tie(succ, diskId, std::ignore) = paramsFirstNVal.ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    napi_valuetype paramsType = napi_undefined;
    napi_typeof(env, funcArg[(int)NARG_POS::SECOND], &paramsType);
    if (paramsType != napi_object) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    PartitionParams params;
    if (!ParsePartitionParams(env, funcArg[(int)NARG_POS::SECOND], params)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    std::string diskIdStr(diskId.get());
    NVal thisVar(env, funcArg.GetThisVar());
    return PromiseVoidOp(env, thisVar, "CreatePartition", [diskIdStr, params]() {
        return DiskManagerClient::GetInstance().CreatePartition(diskIdStr, params);
    });
}

napi_value DeletePartition(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs((int)NARG_CNT::TWO)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    NVal paramsFirstNVal(env, funcArg[(int)NARG_POS::FIRST]);
    if (paramsFirstNVal.val_ == nullptr) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    NVal paramsSecondNVal(env, funcArg[(int)NARG_POS::SECOND]);
    if (paramsSecondNVal.val_ == nullptr) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> diskId;
    std::tie(succ, diskId, std::ignore) = paramsFirstNVal.ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    int32_t partitionNum = 0;
    std::tie(succ, partitionNum) = paramsSecondNVal.ToInt32();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    std::string diskIdStr(diskId.get());
    NVal thisVar(env, funcArg.GetThisVar());
    return PromiseVoidOp(env, thisVar, "DeletePartition", [diskIdStr, partitionNum]() {
        return DiskManagerClient::GetInstance().DeletePartition(diskIdStr, partitionNum);
    });
}

// 辅助函数：解析 FormatParams 参数
static bool ParseFormatParams(napi_env env, napi_value paramsObj, FormatParams &params)
{
    NVal paramsNVal(env, paramsObj);
    if (paramsNVal.val_ == nullptr) {
        return false;
    }
    bool succ = false;
    NVal fsTypeVal = paramsNVal.GetProp("fsType");
    if (fsTypeVal.val_ == nullptr) {
        return false;
    }
    std::unique_ptr<char[]> fsType;
    std::tie(succ, fsType, std::ignore) = fsTypeVal.ToUTF8String();
    if (!succ) {
        return false;
    }
    params.SetFsType(std::string(fsType.get()));
    NVal quickFormatVal = paramsNVal.GetProp("quickFormat");
    if (quickFormatVal.val_ != nullptr) {
        bool quickFormat = true;
        std::tie(succ, quickFormat) = quickFormatVal.ToBool();
        if (succ) {
            params.SetQuickFormat(quickFormat);
        }
    }
    NVal volumeNameVal = paramsNVal.GetProp("volumeName");
    if (volumeNameVal.val_ != nullptr) {
        std::unique_ptr<char[]> volumeName;
        std::tie(succ, volumeName, std::ignore) = volumeNameVal.ToUTF8String();
        if (succ && volumeName != nullptr) {
            params.SetVolumeName(std::string(volumeName.get()));
        }
    }
    return true;
}

// 辅助函数：解析 FormatPartition 的基本参数
static bool ParseFormatPartitionBasicArgs(napi_env env, NFuncArg &funcArg, std::string &diskId, int32_t &partitionNum)
{
    NVal paramsFirstNVal(env, funcArg[(int)NARG_POS::FIRST]);
    if (paramsFirstNVal.val_ == nullptr) {
        return false;
    }
    NVal paramsSecondNVal(env, funcArg[(int)NARG_POS::SECOND]);
    if (paramsSecondNVal.val_ == nullptr) {
        return false;
    }
    bool succ = false;
    std::unique_ptr<char[]> diskIdBuf;
    std::tie(succ, diskIdBuf, std::ignore) = paramsFirstNVal.ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return false;
    }
    diskId = std::string(diskIdBuf.get());
    std::tie(succ, partitionNum) = paramsSecondNVal.ToInt32();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return false;
    }
    return true;
}

napi_value FormatPartition(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs((int)NARG_CNT::THREE)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    std::string diskId;
    int32_t partitionNum = 0;
    if (!ParseFormatPartitionBasicArgs(env, funcArg, diskId, partitionNum)) {
        return nullptr;
    }
    FormatParams params;
    if (!ParseFormatParams(env, funcArg[(int)NARG_POS::THIRD], params)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    NVal thisVar(env, funcArg.GetThisVar());
    return PromiseVoidOp(env, thisVar, "FormatPartition", [diskId, partitionNum, params]() {
        return DiskManagerClient::GetInstance().FormatPartition(diskId, partitionNum, params);
    });
}

// ========== 模块初始化 ==========

// 模块导出的属性描述符
static napi_property_descriptor g_properties[] = {
    // 原有接口
    DECLARE_NAPI_FUNCTION("getAllVolumes", GetAllVolumes),
    DECLARE_NAPI_FUNCTION("mount", Mount),
    DECLARE_NAPI_FUNCTION("unmount", Unmount),
    DECLARE_NAPI_FUNCTION("getVolumeByUuid", GetVolumeByUuid),
    DECLARE_NAPI_FUNCTION("getVolumeById", GetVolumeById),
    DECLARE_NAPI_FUNCTION("setVolumeDescription", SetVolumeDescription),
    DECLARE_NAPI_FUNCTION("format", Format),
    DECLARE_NAPI_FUNCTION("partition", Partition),
    DECLARE_NAPI_FUNCTION("getAllDisks", GetAllDisks),
    DECLARE_NAPI_FUNCTION("getDiskById", GetDiskById),
    // 光驱相关接口
    DECLARE_NAPI_FUNCTION("erase", Erase),
    DECLARE_NAPI_FUNCTION("eject", Eject),
    DECLARE_NAPI_FUNCTION("createIsoImage", CreateIsoImage),
    DECLARE_NAPI_FUNCTION("burn", Burn),
    DECLARE_NAPI_FUNCTION("getOpProcess", GetOpProcess),
    // 分区管理接口 (since 26.0.0)
    DECLARE_NAPI_FUNCTION("getPartitionTable", GetPartitionTable),
    DECLARE_NAPI_FUNCTION("createPartition", CreatePartition),
    DECLARE_NAPI_FUNCTION("deletePartition", DeletePartition),
    DECLARE_NAPI_FUNCTION("formatPartition", FormatPartition),
    DECLARE_NAPI_FUNCTION("isVolumeInUse", IsVolumeInUse),
};

// 辅助函数：导出模块枚举类型
static void ExportModuleEnums(napi_env env, napi_value exports)
{
    napi_value diskType = CreateDiskTypeEnum(env);
    napi_set_named_property(env, exports, "DiskType", diskType);

    napi_value verifyType = CreateVerifyTypeEnum(env);
    napi_set_named_property(env, exports, "VerifyType", verifyType);
}

static napi_value Init(napi_env env, napi_value exports)
{
    napi_define_properties(env, exports, sizeof(g_properties) / sizeof(g_properties[0]), g_properties);
    ExportModuleEnums(env, exports);
    return exports;
}

static napi_module module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "file.volumeManager",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&module);
}

} // namespace ModuleVolumeManager
} // namespace DiskManager
} // namespace OHOS
