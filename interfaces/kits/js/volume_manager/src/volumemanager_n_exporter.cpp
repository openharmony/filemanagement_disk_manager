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

#include "volumemanager_n_exporter.h"

#include "disk.h"
#include "disk_manager_client.h"
#include "disk_manager_napi_errno.h"
#include "disk_manager_napi_utils.h"
#include "n_async/n_async_work_callback.h"
#include "n_async/n_async_work_promise.h"
#include "n_func_arg.h"
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
    const std::string &extra = v.GetExtraInfo();
    if (!extra.empty()) {
        volObj.AddProp("extraInfo", NVal::CreateUTF8String(env, extra).val_);
    }
}

napi_value BuildDiskJSObject(napi_env env, const Disk &d)
{
    NVal diskObj = NVal::CreateObject(env);
    diskObj.AddProp("id", NVal::CreateUTF8String(env, d.GetDiskId()).val_);
    diskObj.AddProp("description", NVal::CreateUTF8String(env, d.GetVendor()).val_);
    diskObj.AddProp("type", NVal::CreateUTF8String(env, d.GetUiType()).val_);
    napi_value capacityVal = nullptr;
    FILEMGMT_CALL_BASE(napi_create_int64(env, d.GetSizeBytes(), &capacityVal), nullptr);
    diskObj.AddProp("capacity", capacityVal);
    diskObj.AddProp("removable", NVal::CreateBool(env, d.IsRemovable()).val_);
    diskObj.AddProp("deviceNode", NVal::CreateUTF8String(env, d.GetSysPath()).val_);
    napi_value vidArr = nullptr;
    FILEMGMT_CALL_BASE(napi_create_array(env, &vidArr), nullptr);
    uint32_t idx = 0;
    for (const auto &vid : d.GetVolumeIds()) {
        napi_set_element(env, vidArr, idx++, NVal::CreateUTF8String(env, vid).val_);
    }
    diskObj.AddProp("volumeIds", vidArr);
    diskObj.AddProp("extInfo", NVal::CreateUTF8String(env, d.GetExtInfo()).val_);
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

napi_value ScheduleVolumeGetOpProcess(napi_env env, const std::string &sid, NVal &thisVar, NFuncArg &funcArg)
{
    struct ProgressGuard {
        std::mutex mtx;
        int32_t value {0};
    };
    auto progress = std::make_shared<ProgressGuard>();
    auto cbExec = [sid, progress]() -> NError {
        int32_t v = 0;
        int32_t errNum =
            DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->GetVolumeOpProcess(sid, v);
        if (errNum != E_OK) {
            return NError(Convert2JsErrNum(errNum));
        }
        std::lock_guard<std::mutex> lock(progress->mtx);
        progress->value = v;
        return NError(ERRNO_NOERR);
    };
    auto cbComplete = [progress](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        int32_t v = 0;
        {
            std::lock_guard<std::mutex> lock(progress->mtx);
            v = progress->value;
        }
        return {NVal::CreateInt32(env, v)};
    };
    constexpr const char *kProcName = "GetOpProcess";
    if (funcArg.GetArgc() == (uint)NARG_CNT::ONE) {
        return NAsyncWorkPromise(env, thisVar).Schedule(kProcName, cbExec, cbComplete).val_;
    }
    NVal cb(env, funcArg[(int)NARG_POS::SECOND]);
    return NAsyncWorkCallback(env, thisVar, cb, FEATURE_STR + __FUNCTION__)
        .Schedule(kProcName, cbExec, cbComplete)
        .val_;
}
} // namespace

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

napi_value GetAllVolumes(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!CheckVolumes(env, info, funcArg)) {
        return nullptr;
    }
    auto volumeInfo = std::make_shared<std::vector<VolumeExternal>>();
    auto cbExec = [volumeInfo]() -> NError {
        int32_t errNum =
            DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->GetAllVolumes(*volumeInfo);
        if (errNum != E_OK) {
            return NError(Convert2JsErrNum(errNum));
        }
        return NError(ERRNO_NOERR);
    };
    auto cbComplete = [volumeInfo](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        napi_value volumeInfoArray = nullptr;
        napi_status status = napi_create_array(env, &volumeInfoArray);
        if (status != napi_ok) {
            return {env, NError(status).GetNapiErr(env)};
        }
        for (size_t i = 0; i < (*volumeInfo).size(); i++) {
            NVal volumeInfoObject = NVal::CreateObject(env);
            FillVolumeJSObject(env, volumeInfoObject, (*volumeInfo)[i]);
            status = napi_set_element(env, volumeInfoArray, i, volumeInfoObject.val_);
            if (status != napi_ok) {
                return {env, NError(status).GetNapiErr(env)};
            }
        }
        return {NVal(env, volumeInfoArray)};
    };
    NVal thisVar(env, funcArg.GetThisVar());
    if (funcArg.GetArgc() == (uint)NARG_CNT::ZERO) {
        return NAsyncWorkPromise(env, thisVar).Schedule("GetAllVolumes", cbExec, cbComplete).val_;
    } else {
        NVal cb(env, funcArg[(int)NARG_POS::FIRST]);
        return NAsyncWorkCallback(env, thisVar, cb, FEATURE_STR + __FUNCTION__)
            .Schedule("GetAllVolumes", cbExec, cbComplete)
            .val_;
    }
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
        int32_t result = DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->Mount(volumeIdString);
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
        int32_t result = DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->Unmount(volumeIdString);
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
        int32_t errNum = DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->GetVolumeByUuid(
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
        int32_t errNum = DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->GetVolumeById(
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

    bool succ = false;
    std::unique_ptr<char[]> uuid;
    std::unique_ptr<char[]> description;
    std::tie(succ, uuid, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    std::tie(succ, description, std::ignore) = NVal(env, funcArg[(int)NARG_POS::SECOND]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    std::string uuidString(uuid.get());
    std::string descStr(description.get());
    auto cbExec = [uuidString, descStr]() -> NError {
        int32_t result = DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->SetVolumeDescription(
            uuidString, descStr);
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

    std::string procedureName = "SetVolumeDescription";
    NVal thisVar(env, funcArg.GetThisVar());
    if (funcArg.GetArgc() == (uint)NARG_CNT::TWO) {
        return NAsyncWorkPromise(env, thisVar).Schedule(procedureName, cbExec, cbComplete).val_;
    } else {
        NVal cb(env, funcArg[(int)NARG_POS::THIRD]);
        return NAsyncWorkCallback(env, thisVar, cb, FEATURE_STR + __FUNCTION__)
            .Schedule(procedureName, cbExec, cbComplete)
            .val_;
    }
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

    bool succ = false;
    std::unique_ptr<char[]> volumeId;
    std::unique_ptr<char[]> fsType;
    std::tie(succ, volumeId, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    std::tie(succ, fsType, std::ignore) = NVal(env, funcArg[(int)NARG_POS::SECOND]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    std::string volumeIdString(volumeId.get());
    std::string fsTypeString(fsType.get());
    auto cbExec = [volumeIdString, fsTypeString]() -> NError {
        int32_t result =
            DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->Format(volumeIdString, fsTypeString);
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

    std::string procedureName = "Format";
    NVal thisVar(env, funcArg.GetThisVar());
    if (funcArg.GetArgc() == (uint)NARG_CNT::TWO) {
        return NAsyncWorkPromise(env, thisVar).Schedule(procedureName, cbExec, cbComplete).val_;
    } else {
        NVal cb(env, funcArg[(int)NARG_POS::THIRD]);
        return NAsyncWorkCallback(env, thisVar, cb, FEATURE_STR + __FUNCTION__)
            .Schedule(procedureName, cbExec, cbComplete)
            .val_;
    }
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

    bool succ = false;
    std::unique_ptr<char[]> diskId;
    int32_t type;
    std::tie(succ, diskId, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    std::tie(succ, type) = NVal(env, funcArg[(int)NARG_POS::SECOND]).ToInt32();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    std::string diskIdString(diskId.get());
    auto cbExec = [diskIdString, type]() -> NError {
        int32_t result =
            DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->Partition(diskIdString, type);
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

    std::string procedureName = "Partition";
    NVal thisVar(env, funcArg.GetThisVar());
    if (funcArg.GetArgc() == (uint)NARG_CNT::TWO) {
        return NAsyncWorkPromise(env, thisVar).Schedule(procedureName, cbExec, cbComplete).val_;
    } else {
        NVal cb(env, funcArg[(int)NARG_POS::THIRD]);
        return NAsyncWorkCallback(env, thisVar, cb, FEATURE_STR + __FUNCTION__)
            .Schedule(procedureName, cbExec, cbComplete)
            .val_;
    }
}

napi_value GetAllDisks(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!CheckVolumes(env, info, funcArg)) {
        return nullptr;
    }
    auto disks = std::make_shared<std::vector<Disk>>();
    auto cbExec = [disks]() -> NError {
        int32_t errNum = DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->GetAllDisks(*disks);
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
    if (funcArg.GetArgc() == (uint)NARG_CNT::ZERO) {
        return NAsyncWorkPromise(env, thisVar).Schedule("GetAllDisks", cbExec, cbComplete).val_;
    }
    NVal cb(env, funcArg[(int)NARG_POS::FIRST]);
    return NAsyncWorkCallback(env, thisVar, cb, FEATURE_STR + __FUNCTION__)
        .Schedule("GetAllDisks", cbExec, cbComplete)
        .val_;
}

napi_value GetDiskById(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!CheckMount(env, info, funcArg)) {
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> diskId;
    std::tie(succ, diskId, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    std::string diskIdStr(diskId.get());
    auto disk = std::make_shared<Disk>();
    auto cbExec = [diskIdStr, disk]() -> NError {
        int32_t errNum =
            DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->GetDiskById(diskIdStr, *disk);
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
    std::string procedureName = "GetDiskById";
    NVal thisVar(env, funcArg.GetThisVar());
    if (funcArg.GetArgc() == (uint)NARG_CNT::ONE) {
        return NAsyncWorkPromise(env, thisVar).Schedule(procedureName, cbExec, cbComplete).val_;
    }
    NVal cb(env, funcArg[(int)NARG_POS::SECOND]);
    return NAsyncWorkCallback(env, thisVar, cb, FEATURE_STR + __FUNCTION__)
        .Schedule(procedureName, cbExec, cbComplete)
        .val_;
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

napi_value Erase(napi_env env, napi_callback_info info)
{
    if (!IsSystemApp()) {
        NError(E_PERMISSION_SYS).ThrowErr(env);
        return nullptr;
    }
    NFuncArg funcArg(env, info);
    /* 与 storage_manager kits_impl/volumemanager_n_exporter.cpp::Erase 一致：1/2 参数（Promise 或 AsyncCallback） */
    if (!funcArg.InitArgs((int)NARG_CNT::ONE, (int)NARG_CNT::TWO)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> vid;
    std::tie(succ, vid, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    std::string sid(vid.get());
    auto cbExec = [sid]() -> NError {
        int32_t errNum = DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->EraseVolume(sid);
        if (errNum != E_OK) {
            return NError(Convert2JsErrNum(errNum));
        }
        return NError(ERRNO_NOERR);
    };
    auto cbComplete = [](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return {NVal::CreateUndefined(env)};
    };
    std::string procedureName = "Erase";
    NVal thisVar(env, funcArg.GetThisVar());
    if (funcArg.GetArgc() == (uint)NARG_CNT::ONE) {
        return NAsyncWorkPromise(env, thisVar).Schedule(procedureName, cbExec, cbComplete).val_;
    }
    NVal cb(env, funcArg[(int)NARG_POS::SECOND]);
    return NAsyncWorkCallback(env, thisVar, cb, FEATURE_STR + __FUNCTION__)
        .Schedule(procedureName, cbExec, cbComplete)
        .val_;
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
    std::unique_ptr<char[]> vid;
    std::tie(succ, vid, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    std::string sid(vid.get());
    auto cbExec = [sid]() -> NError {
        int32_t errNum = DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->EjectVolume(sid);
        if (errNum != E_OK) {
            return NError(Convert2JsErrNum(errNum));
        }
        return NError(ERRNO_NOERR);
    };
    auto cbComplete = [](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return {NVal::CreateUndefined(env)};
    };
    std::string procedureName = "Eject";
    NVal thisVar(env, funcArg.GetThisVar());
    if (funcArg.GetArgc() == (uint)NARG_CNT::ONE) {
        return NAsyncWorkPromise(env, thisVar).Schedule(procedureName, cbExec, cbComplete).val_;
    }
    NVal cb(env, funcArg[(int)NARG_POS::SECOND]);
    return NAsyncWorkCallback(env, thisVar, cb, FEATURE_STR + __FUNCTION__)
        .Schedule(procedureName, cbExec, cbComplete)
        .val_;
}

napi_value CreateIsoImage(napi_env env, napi_callback_info info)
{
    if (!IsSystemApp()) {
        NError(E_PERMISSION_SYS).ThrowErr(env);
        return nullptr;
    }
    NFuncArg funcArg(env, info);
    /* 与 storage_manager：2/3 参数 */
    if (!funcArg.InitArgs((int)NARG_CNT::TWO, (int)NARG_CNT::THREE)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> vid;
    std::unique_ptr<char[]> fp;
    std::tie(succ, vid, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    std::tie(succ, fp, std::ignore) = NVal(env, funcArg[(int)NARG_POS::SECOND]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    std::string sv(vid.get());
    std::string path(fp.get());
    auto cbExec = [sv, path]() -> NError {
        int32_t result =
            DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->CreateIsoImage(sv, path);
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
    std::string procedureName = "CreateIsoImage";
    NVal thisVar(env, funcArg.GetThisVar());
    if (funcArg.GetArgc() == (uint)NARG_CNT::TWO) {
        return NAsyncWorkPromise(env, thisVar).Schedule(procedureName, cbExec, cbComplete).val_;
    }
    NVal cb(env, funcArg[(int)NARG_POS::THIRD]);
    return NAsyncWorkCallback(env, thisVar, cb, FEATURE_STR + __FUNCTION__)
        .Schedule(procedureName, cbExec, cbComplete)
        .val_;
}

napi_value Burn(napi_env env, napi_callback_info info)
{
    if (!IsSystemApp()) {
        NError(E_PERMISSION_SYS).ThrowErr(env);
        return nullptr;
    }
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs((int)NARG_CNT::TWO, (int)NARG_CNT::TWO)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> vid;
    std::tie(succ, vid, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    napi_valuetype wantType = napi_undefined;
    NVal wantArg(env, funcArg[(int)NARG_POS::SECOND]);
    if (napi_typeof(env, wantArg.val_, &wantType) != napi_ok || wantType != napi_object) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    napi_value wantVal = wantArg.val_;
    std::string opts = BuildBurnOptionsLines(env, wantVal);
    std::string sv(vid.get());
    NVal thisVar(env, funcArg.GetThisVar());
    return PromiseVoidOp(env, thisVar, "Burn", [sv, opts]() {
        return DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->BurnVolume(sv, opts);
    });
}

napi_value GetOpProcess(napi_env env, napi_callback_info info)
{
    if (!IsSystemApp()) {
        NError(E_PERMISSION_SYS).ThrowErr(env);
        return nullptr;
    }
    NFuncArg funcArg(env, info);
    /* 与 storage_manager GetOpticalDriveOpsProgress：1/2 参数 */
    if (!funcArg.InitArgs((int)NARG_CNT::ONE, (int)NARG_CNT::TWO)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> vid;
    std::tie(succ, vid, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    NVal thisVar(env, funcArg.GetThisVar());
    return ScheduleVolumeGetOpProcess(env, std::string(vid.get()), thisVar, funcArg);
}

napi_value VerifyBurnData(napi_env env, napi_callback_info info)
{
    if (!IsSystemApp()) {
        NError(E_PERMISSION_SYS).ThrowErr(env);
        return nullptr;
    }
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs((int)NARG_CNT::TWO, (int)NARG_CNT::TWO)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    bool succ = false;
    std::unique_ptr<char[]> vid;
    std::tie(succ, vid, std::ignore) = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    int32_t vType = 0;
    std::tie(succ, vType) = NVal(env, funcArg[(int)NARG_POS::SECOND]).ToInt32();
    if (!succ) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    std::string sv(vid.get());
    NVal thisVar(env, funcArg.GetThisVar());
    return PromiseVoidOp(env, thisVar, "VerifyBurnData", [sv, vType]() {
        return DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->VerifyBurnData(sv, vType);
    });
}
} // namespace ModuleVolumeManager
} // namespace DiskManager
} // namespace OHOS
