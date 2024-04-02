# API说明

- AX_SKEL_Init
- AX_SKEL_DeInit
- AX_SKEL_GetCapability
- AX_SKEL_GetVersion
- AX_SKEL_Create
- AX_SKEL_Destroy
- AX_SKEL_GetConfig
- AX_SKEL_SetConfig
- AX_SKEL_RegisterResultCallback
- AX_SKEL_SendFrame
- AX_SKEL_GetResult
- AX_SKEL_Release

## AX_SKEL_Init
### 【描述】
初始化SKEL模块
### 【语法】
AX_S32 AX_SKEL_Init(const AX_SKEL_INIT_PARAM_T *pstParam)
### 【参数】
| 参数名称     | 描述                                                   | 输入/输出 |
|----------|------------------------------------------------------|-------|
| pstParam | 初始化参数，不能为空，参照[ax_skel_type.h](../inc/ax_skel_type.h) | 输入    |
### 【返回】
| 返回值 | 描述                                         |
|-----|--------------------------------------------|
| 非0  | 失败，参照[ax_skel_err.h](../inc/ax_skel_err.h) |
| 0   | 成功                                         |
### 【注意】
### 【示例】
[hvcfp_demo.cpp](../demo/hvcfp_demo.cpp)


## AX_SKEL_DeInit
### 【描述】
销毁SKEL模块
### 【语法】
AX_S32 AX_SKEL_DeInit(AX_VOID)
### 【参数】
### 【返回】
| 返回值 | 描述                                         |
|-----|--------------------------------------------|
| 非0  | 失败，参照[ax_skel_err.h](../inc/ax_skel_err.h) |
| 0   | 成功                                         |
### 【注意】
与AX_SKEL_Init成对使用。
### 【示例】
[hvcfp_demo.cpp](../demo/hvcfp_demo.cpp)


## AX_SKEL_GetCapability
### 【描述】
获取SKEL可用的Pipeline
### 【语法】
AX_S32 AX_SKEL_GetCapability(const AX_SKEL_CAPABILITY_T **ppstCapability)
### 【参数】
| 参数名称           | 描述                                        | 输入/输出 |
|----------------|-------------------------------------------|-------|
| ppstCapability | 参照[ax_skel_type.h](../inc/ax_skel_type.h) | 输出    |
### 【返回】
| 返回值 | 描述                                         |
|-----|--------------------------------------------|
| 非0  | 失败，参照[ax_skel_err.h](../inc/ax_skel_err.h) |
| 0   | 成功                                         |
### 【注意】
### 【示例】
[ax_skel_getcap.cpp](../demo/ax_skel_getcap.cpp)


## AX_SKEL_GetVersion
### 【描述】
获取SKEL版本
### 【语法】
AX_S32 AX_SKEL_GetVersion(const AX_SKEL_VERSION_INFO_T **ppstVersion)
### 【参数】
| 参数名称        | 描述                                        | 输入/输出 |
|-------------|-------------------------------------------|-------|
| ppstVersion | 参照[ax_skel_type.h](../inc/ax_skel_type.h) | 输出    |
### 【返回】
| 返回值 | 描述                                         |
|-----|--------------------------------------------|
| 非0  | 失败，参照[ax_skel_err.h](../inc/ax_skel_err.h) |
| 0   | 成功                                         |
### 【注意】
### 【示例】
[ax_skel_version.cpp](../demo/ax_skel_version.cpp)


## AX_SKEL_Create
### 【描述】
创建Pipeline句柄
### 【语法】
AX_S32 AX_SKEL_Create(const AX_SKEL_HANDLE_PARAM_T *pstParam, AX_SKEL_HANDLE *pHandle)
### 【参数】
| 参数名称     | 描述                                        | 输入/输出 |
|----------|-------------------------------------------|-------|
| pstParam | 参照[ax_skel_type.h](../inc/ax_skel_type.h) | 输入    |
| pHandle  | Pipeline句柄                                | 输出    |
### 【返回】
| 返回值 | 描述                                         |
|-----|--------------------------------------------|
| 非0  | 失败，参照[ax_skel_err.h](../inc/ax_skel_err.h) |
| 0   | 成功                                         |
### 【注意】
### 【示例】
[hvcfp_demo.cpp](../demo/hvcfp_demo.cpp)


## AX_SKEL_Destroy
### 【描述】
销毁Pipeline句柄
### 【语法】
AX_S32 AX_SKEL_Destroy(AX_SKEL_HANDLE handle)
### 【参数】
| 参数名称   | 描述         | 输入/输出 |
|--------|------------|-------|
| handle | Pipeline句柄 | 输入    |
### 【返回】
| 返回值 | 描述                                         |
|-----|--------------------------------------------|
| 非0  | 失败，参照[ax_skel_err.h](../inc/ax_skel_err.h) |
| 0   | 成功                                         |
### 【注意】
### 【示例】
[hvcfp_demo.cpp](../demo/hvcfp_demo.cpp)


## AX_SKEL_GetConfig
### 【描述】
获取Pipeline配置
### 【语法】
AX_S32 AX_SKEL_GetConfig(AX_SKEL_HANDLE handle, const AX_SKEL_CONFIG_T **ppstConfig)
### 【参数】
| 参数名称       | 描述                                        | 输入/输出 |
|------------|-------------------------------------------|-------|
| handle     | Pipeline句柄                                | 输入    |
| ppstConfig | 参照[ax_skel_type.h](../inc/ax_skel_type.h) | 输出    |
### 【返回】
| 返回值 | 描述                                         |
|-----|--------------------------------------------|
| 非0  | 失败，参照[ax_skel_err.h](../inc/ax_skel_err.h) |
| 0   | 成功                                         |
### 【注意】
### 【示例】
[hvcfp_demo.cpp](../demo/hvcfp_demo.cpp)


## AX_SKEL_SetConfig
### 【描述】
设置Pipeline配置
### 【语法】
AX_S32 AX_SKEL_SetConfig(AX_SKEL_HANDLE handle, const AX_SKEL_CONFIG_T *pstConfig)
### 【参数】
| 参数名称      | 描述                                        | 输入/输出 |
|-----------|-------------------------------------------|-------|
| handle    | Pipeline句柄                                | 输入    |
| pstConfig | 参照[ax_skel_type.h](../inc/ax_skel_type.h) | 输入    |
### 【返回】
| 返回值 | 描述                                         |
|-----|--------------------------------------------|
| 非0  | 失败，参照[ax_skel_err.h](../inc/ax_skel_err.h) |
| 0   | 成功                                         |
### 【注意】
### 【示例】
[hvcfp_demo.cpp](../demo/hvcfp_demo.cpp)


## AX_SKEL_RegisterResultCallback
### 【描述】
注册Pipeline结果回调
### 【语法】
AX_S32 AX_SKEL_RegisterResultCallback(AX_SKEL_HANDLE handle, AX_SKEL_RESULT_CALLBACK_FUNC callback, AX_VOID *pUserData)
### 【参数】
| 参数名称      | 描述                                              | 输入/输出 |
|-----------|-------------------------------------------------|-------|
| handle    | Pipeline句柄                                      | 输入    |
| callback  | 回调函数， 参照[ax_skel_type.h](../inc/ax_skel_type.h) | 输入    |
| pUserData | 回调自定义数据                                         | 输入    |
### 【返回】
| 返回值 | 描述                                         |
|-----|--------------------------------------------|
| 非0  | 失败，参照[ax_skel_err.h](../inc/ax_skel_err.h) |
| 0   | 成功                                         |
### 【注意】
如果算法结果通过回调获取，则不应再使用AX_SKEL_GetResult
### 【示例】
[hvcfp_demo.cpp](../demo/hvcfp_demo.cpp)


## AX_SKEL_SendFrame
### 【描述】
向Pipeline发送图像
### 【语法】
AX_S32 AX_SKEL_SendFrame(AX_SKEL_HANDLE handle, const AX_SKEL_FRAME_T *pstFrame, AX_S32 nTimeout)
### 【参数】
| 参数名称     | 描述                                               | 输入/输出 |
|----------|--------------------------------------------------|-------|
| handle   | Pipeline句柄                                       | 输入    |
| pstFrame | 图像结构体， 参照[ax_skel_type.h](../inc/ax_skel_type.h) | 输入    |
| nTimeout | 阻塞时长，-1表示阻塞，0表示即时返回，>0时表示等待毫秒数                   | 输入    |
### 【返回】
| 返回值 | 描述                                         |
|-----|--------------------------------------------|
| 非0  | 失败，参照[ax_skel_err.h](../inc/ax_skel_err.h) |
| 0   | 成功                                         |
### 【注意】
### 【示例】
[hvcfp_demo.cpp](../demo/hvcfp_demo.cpp)


## AX_SKEL_GetResult
### 【描述】
获取Pipeline算法结果，需要与AX_SKEL_Release成对使用。
### 【语法】
AX_S32 AX_SKEL_GetResult(AX_SKEL_HANDLE handle, AX_SKEL_RESULT_T **ppstResult, AX_S32 nTimeout)
### 【参数】
| 参数名称       | 描述                                                 | 输入/输出 |
|------------|----------------------------------------------------|-------|
| handle     | Pipeline句柄                                         | 输入    |
| ppstResult | 算法结果结构体， 参照[ax_skel_type.h](../inc/ax_skel_type.h) | 输出    |
| nTimeout   | 阻塞时长，-1表示阻塞，0表示即时返回，>0时表示等待毫秒数                     | 输入    |
### 【返回】
| 返回值 | 描述                                         |
|-----|--------------------------------------------|
| 非0  | 失败，参照[ax_skel_err.h](../inc/ax_skel_err.h) |
| 0   | 成功                                         |
### 【注意】
### 【示例】
[hvcfp_demo.cpp](../demo/hvcfp_demo.cpp)


## AX_SKEL_Release
### 【描述】
释放算法结果
### 【语法】
AX_S32 AX_SKEL_Release(AX_VOID *p)
### 【参数】
| 参数名称       | 描述                                                 | 输入/输出 |
|------------|----------------------------------------------------|-------|
| p          | AX_SKEL_RESULT_T*类型的算法结果                           | 输入    |
### 【返回】
| 返回值 | 描述                                         |
|-----|--------------------------------------------|
| 非0  | 失败，参照[ax_skel_err.h](../inc/ax_skel_err.h) |
| 0   | 成功                                         |
### 【注意】
如果使用回调方式获取算法结果，则不需要通过此接口释放
### 【示例】
[hvcfp_demo.cpp](../demo/hvcfp_demo.cpp)