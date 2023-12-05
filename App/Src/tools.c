#include "tools.h"

void* GetFunc(const char* FuncName){
    static void* handle = NULL;
    static int bRecord = 0;
    if(handle==NULL){
        handle = dlopen("./lib/libitop_sdk.so",RTLD_LAZY);
    }
    if(handle){
        void* func = dlsym(handle,FuncName);
        if(func){
            if(!bRecord){
                bRecord = 1;
            }
        }
        else{
            if(!bRecord){
                dlclose(handle);
                handle = NULL;
            }
        }
        return func;
    }
    return NULL;
}


DH_Int32 app_ptz_init(){
    typedef int (*PTZ_getPtzChnNum)();
    typedef int (*PTZ_getChnCaps)(int,DHOP_PTZ_Caps*);
    typedef int (*PTZ_open)(DHOP_PTZ_OpenParam*,DH_Handle*);
    DHOP_PTZ_Caps caps;
    DH_Int32 ret = -1;
    DHOP_PTZ_OpenParam pParm = {0,};
    PTZ_getPtzChnNum DHOP_PTZ_getPtzChnNumFunc = (PTZ_getPtzChnNum)GetFunc("DHOP_PTZ_getPtzChnNum");
    PTZ_getChnCaps DHOP_PTZ_getChnCapsFunc = (PTZ_getChnCaps)GetFunc("DHOP_PTZ_getChnCaps");
    PTZ_open       DHOP_PTZ_openFunc = (PTZ_open)GetFunc("DHOP_PTZ_open");
    ret = DHOP_PTZ_openFunc(&pParm,&(g_app_global.hPTZ));
    return ret;
}

DH_Int32 app_ptz_deinit(){
    typedef int (*PTZ_close)(DH_Handle*);
    PTZ_close DHOP_PTZ_closeFunc = (PTZ_close)GetFunc("DHOP_PTZ_close");
    DH_Int32 ret = -1;

    ret = DHOP_PTZ_closeFunc(&(g_app_global.hPTZ));
    return ret;
}


// 对AI检测坐标进行转换
DH_Int32 app_size_limit(DH_Int32 pix, DH_Int32 max)
{
    if (pix < 0)
    {
        return 0;
    }
    if(pix > max)
    {
        return max-1;
    }
    return pix;
}

int app_net_init() {
    DH_Int32                ret;
    struct sockaddr_in      sever_addr;

    g_app_global.hNet = socket(AF_INET, SOCK_STREAM, 0);
    if (g_app_global.hNet < 0)
    {
        perror("socket failed");
        g_app_global.hNet = -1;
        return -1;
    }
    DHOP_LOG_INFO("create socket %d\n", g_app_global.hNet);

    memset(&sever_addr, 0, sizeof(sever_addr));

    sever_addr.sin_family       = AF_INET;
    sever_addr.sin_port         = htons(g_app_config.net_port);
    sever_addr.sin_addr.s_addr  = inet_addr(g_app_config.net_ip);

    ret = connect(g_app_global.hNet, (struct sockaddr *)&sever_addr, sizeof(sever_addr));
    if (ret != 0)
    {
        perror("connect failed");

        close(g_app_global.hNet);
        g_app_global.hNet = -1;
    }
    else {
        DHOP_LOG_INFO("connect success: %d\n", g_app_global.hNet);
    }

    return ret;
}

int app_net_deinit() {
    if (g_app_global.hNet > 0) {
        close(g_app_global.hNet);
        g_app_global.hNet = -1;
    }
    return 0;
}

int app_net_reinit() {
    app_net_deinit();
    return app_net_init();
}

int app_enc_init() {
    DH_Int32                ret;
    DHOP_YUV_CapInfo        yuvCap;
    DHOP_VENC_CapsInfo      encCap;
    DHOP_VENC_CreateParam   createParam;

    ret = DHOP_YUV_getChnCaps(0, &yuvCap);
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("DHOP_YUV_getChnCaps fail with %#x\n", ret);
        return ret;
    }

    memset(&createParam, 0, sizeof(createParam));
    createParam.cbSize = sizeof(createParam);
    createParam.type = DHOP_VENC_YUV_TO_JPEG;
    createParam.max_width = yuvCap.maxWidth;
    createParam.max_height = yuvCap.maxHeight;
    createParam.format = DHOP_YUV_FMT_420SP_VU;

    ret = DHOP_VENC_getCaps(&encCap);

    g_app_global.encAlignW = encCap.alignWidth;
    g_app_global.encAlignH = encCap.alignHeight;

    // 创建编码器
    ret = DHOP_VENC_create(&createParam, &(g_app_global.hVenc));
    if (ret != DHOP_SUCCESS) {
        DHOP_LOG_ERROR("DHOP_VENC_create fail with %#x\n", ret);
        return ret;
    }

    return 0;
}

int app_enc_deinit() {
    DHOP_VENC_destroy(&(g_app_global.hVenc));
    return 0;
}

// YUV的初始化
DH_Int32 app_yuv_init()
{
    DH_Int32                ret = -1;
    DHOP_YUV_Option         yuvOption;
    DHOP_YUV_CapInfo        yuvCap;
    DHOP_YUV_OpenParam      yuvOpenPrm;
    DHOP_YUV_FormatParam    yuvFmtPrm;
    DH_Int32                yuvChn = 0;

    memset(&yuvCap, 0, sizeof(DHOP_YUV_CapInfo));
    memset(&yuvOpenPrm, 0, sizeof(yuvOpenPrm));
    // 1.获取YUV通道能力
    ret = DHOP_YUV_getChnCaps(yuvChn, &yuvCap);
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("DHOP_YUV_getChnCaps fail with %#x\n", ret);
        return ret;
    }

    yuvOpenPrm.channel = 0;
    // 2.开启YUV通道
    ret = DHOP_YUV_open(&yuvOpenPrm, &(g_app_global.hYuv));
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("DHOP_YUV_open fail with %#x\n", ret);
        return ret;
    }

    memset(&yuvFmtPrm, 0, sizeof(yuvFmtPrm));
    yuvFmtPrm.format = DHOP_YUV_FMT_420SP_VU;
    yuvFmtPrm.fps    = 2;
    yuvFmtPrm.width  = yuvCap.maxWidth;
    yuvFmtPrm.height = yuvCap.maxHeight;
    // 3.设置YUV通道格式参数
    ret = DHOP_YUV_setFormat(g_app_global.hYuv, &yuvFmtPrm);
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("DHOP_YUV_setFormat fail with %#x\n", ret);
        DHOP_YUV_close(&g_app_global.hYuv);
        return ret;
    }

    memset(&yuvOption, 0, sizeof(yuvOption));
    yuvOption.cbSize = sizeof(yuvOption);
    yuvOption.type = DHOP_YUV_OPT_DEPTH;
    yuvOption.option.depth = 1;
    // 4.设置YUV可选配置
    ret = DHOP_YUV_setOption(g_app_global.hYuv, &yuvOption);
    if(DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("DHOP_YUV_setOption fail with %#x\n", ret);
        DHOP_YUV_close(&g_app_global.hYuv);
        return ret;
    }
}

// 关闭YUV
DH_Int32 app_yuv_deinit()
{
    DHOP_YUV_close(&(g_app_global.hYuv));
    return DHOP_SUCCESS;
}



// 将检测的结果发送给服务器端
int app_result_send(DHOP_VENC_Result* image, send_infos* result) {
    DH_Int32                ret = -1;

    if (g_app_global.hNet > 0) {
        result->pic_size = image->size;
        ret = send(g_app_global.hNet, &result, sizeof(result), 0);
        if (ret < 0) {
            perror("send head failed:");
            app_net_reinit();
            return -2;
        }

        ret = send(g_app_global.hNet, image->virAddr, image->size, 0);
        if (ret < 0) {
            perror("send image failed:");
            app_net_reinit();
            return -3;
        }

        return 0;
    }

    return -1;
}

// 对算法检测的数据进行编码并发送给桌面端
DH_Int32 app_result_snap(send_infos* result, DHOP_YUV_FrameData2* frame) {
    DH_Int32                ret = -1;
    DHOP_VENC_ReqInfo2      encReq;
    DHOP_VENC_Result        encResult;

    memset(&encReq, 0, sizeof(encReq));
    encReq.cbSize       = sizeof(encReq);
    encReq.quality      = DHOP_VENC_JPEG_QUALITY_DEFAULT;
    encReq.region.lt.x  = 0;
    encReq.region.lt.y  = 0;
    encReq.region.rb.x  = frame->data.width;
    encReq.region.rb.y  = frame->data.height;
    encReq.data         = frame;
    encReq.timeout      = 200;
    ret = DHOP_VENC_sendRequest(g_app_global.hVenc, &encReq);
    if (ret != DHOP_SUCCESS) {
        DHOP_LOG_WARN("Send enc reqeust failed!\n");
        return ret;
    }
    else {
        memset(&encResult, 0, sizeof(encResult));
        encResult.cbSize  = sizeof(encResult);
        encResult.pts     = encReq.data->pts;
        encResult.timeout = 200;
        ret = DHOP_VENC_getResult(g_app_global.hVenc, &encResult);
        if (ret != DHOP_SUCCESS) {
            DHOP_LOG_WARN("Get enc result failed!\n");
        }

        app_result_send(&encResult, result);

        ret = DHOP_VENC_releaseResult(g_app_global.hVenc, &encResult);
        if (ret != DHOP_SUCCESS) {
            DHOP_LOG_WARN("Release enc result failed!\n");
        }
    }   
    return ret;
}

int app_ai_init() {

    DH_Int32                ret = -1;
    DHOP_AI_NNX_Version     ver;
    char modelkeystr[] = "gGQaZICol8tv2BqcbQ0UZG9kXmRvZBnsPWQDZMRkkmSg/G8xkNtvZG5k5a1vZG/81a00yXIgcMpvDpFkOmRMYg==";

    ver.byte_size = sizeof(DHOP_AI_NNX_Version);
    // 获取引擎版本号
    ret = DHOP_AI_NNX_getVersion(&ver);
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("DHOP_AI_NNX_getVersion %#x\n", ret);
        return DHOP_FAILED;
    }
    DHOP_LOG_INFO("version:%s\n", ver.version);

    // AI初始化
    ret = DHOP_AI_init();
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("dhop ai init fail %#x\n", ret);
        return DHOP_FAILED;
    }
    DHOP_LOG_INFO("DHOP_AI_init success\n");

    // 创建引擎
    ret = DHOP_AI_NNX_create(&(g_app_global.hNNX), g_modelfile, modelkeystr);
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("dhop ai nnx create fail#x\n", ret);
        return DHOP_FAILED;
    }

    return 0;
}

int app_ai_deinit() {
    // 销毁算法模型句柄
    DHOP_AI_NNX_destroy(g_app_global.hNNX);
    // AI去初始化
    DHOP_AI_unInit();

    return 0;
}

// AI的运行
DH_Int32 app_ai_process(DHOP_AI_NNX_Handle hNNX, DHOP_YUV_FrameData2 * frame, send_infos* results)
{
    DH_Int32                ret = -1, i, k;
    DH_Ptr                  ptrs[2];
    DH_Ptr                  ptrs_HW[2];
    DH_Int32                strides[2];
    DHOP_AI_IMG_Handle      hImg;
    DH_Uint32               type = DHOP_AI_NNX_RESULT_TYPE_MAX;
    DHOP_AI_MAT_Handle      yoloMat;
    DH_Int32                h,start;
    DHOP_AI_NNX_ResultYolo *yolo_result;

    g_app_global.resultNum = 0;

    ptrs[0]     = frame->data.virAddr.nv21.y;
    ptrs[1]     = frame->data.virAddr.nv21.vu;
    ptrs_HW[0]  = frame->data.phyAddr.nv12.y;
    ptrs_HW[1]  = frame->data.phyAddr.nv21.vu;
    strides[0]  = frame->data.stride[0];
    strides[1]  = frame->data.width;

    //split 的逻辑

    // 创建DHOP_AI_IMG_Handle的句柄，需要DHOP_AI_IMG_destroy()来释放img的内存
    ret = DHOP_AI_IMG_create(&hImg,
                             frame->data.width,
                             frame->data.height,
                             DHOP_AI_IMG_CS_YUV420SP_VU,
                             DHOP_AI_IMG_8U,
                             ptrs,
                             ptrs_HW,
                             strides,
                             2);
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("creat DHOP_AI_IMG_Handle fail\n");
        goto err0;
    }

    // 设置图片输入
    ret = DHOP_AI_NNX_setInputImg(hNNX, (const DH_String)"image", &hImg, 1);
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("dhop ai set input img fail");
        goto err1;
    }

    // AI运行
    ret = DHOP_AI_NNX_run(hNNX);
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("dhop ai run fail");
        goto err1;
    }

    // 获取AI检测结果
    type = DHOP_AI_NNX_RESULT_TYPE_MAX;
    ret = DHOP_AI_NNX_getResult(hNNX, (const DH_String)"result", &type, &yoloMat);
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("dhop ai get result fail");
        goto err1;
    }
    if (type != DHOP_AI_NNX_RESULT_TYPE_YOLO)
    {
        DHOP_LOG_ERROR("nnx result type is not yolo\n");
        goto err1;
    }

    // 获取矩阵在指定维度上的活动范围。
    ret = DHOP_AI_MAT_getActiveRange(yoloMat, 0, &start, &h);
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("get rect num fail\n");
        goto err1;
    }

    yolo_result = (DHOP_AI_NNX_ResultYolo *)DHOP_AI_MAT_ptr2(yoloMat, NULL);
    if (NULL == yolo_result) {
        goto err1;
    }

    int k = 0;

    for (i = start; i < start+h; i++)
    {
        // 过滤置信度极小的值
        if ( yolo_result[i].prob > 0.2 )
        {
            if (k < APP_MAX_AI_RESULT_NUM) {
                // 算法输出的是0~1的浮点数据，要转换成YUV frame的宽高坐标
                results->bboxes[k].actual.lt.x = app_size_limit((yolo_result[i].x - yolo_result[i].w/2) * frame->data.width, frame->data.width);
                results->bboxes[k].actual.lt.y = app_size_limit((yolo_result[i].y - yolo_result[i].h/2) * frame->data.height, frame->data.height);
                results->bboxes[k].actual.rb.x = app_size_limit((yolo_result[i].x + yolo_result[i].w/2) * frame->data.width, frame->data.width);
                results->bboxes[k].actual.rb.y  = app_size_limit((yolo_result[i].y + yolo_result[i].h/2) * frame->data.height, frame->data.height);
                k++;
            }
            else {
                break;
            }
        }
    }
    results->pest_num = k;

err1:
    /// 摧毁DHOP_AI_IMG_Handle句柄
    ret = DHOP_AI_IMG_destroy(hImg);
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("DHOP_AI_IMG_destroy fail\n");
    }

err0:
    return ret;
}

// url解码
int app_http_urldecode(const char* src, char* outbuf, int size) {
    enum _c_t {
        normal  = 0,
        percent = 16,
        space   = 17,
    };
    int  i, k;
    char byte[128] = {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0, 16,  0,  0,  0,  0,  0, 17,  0,  0,  0,  0,
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0,  0,  0,  0,  0,  0,
        0, 10, 11, 12, 13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0, 10, 11, 12, 13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    };

    for (i = 0, k = 0; src[i] != '\0'; i++) {
        if (k+1 >= size) {
            break;
        }

        switch (byte[src[i]]) {
        case percent:
            outbuf[k++] = (byte[src[i + 1]] << 4) | (byte[src[i + 2]]);
            i += 2;
            break;
        case space:
            outbuf[k++] = ' ';
            break;
        default:
            outbuf[k++] = src[i];
            break;
        }
    }
    outbuf[k++] = 0;

    return k;
}

DH_Int32 app_http_on_request(const DHOP_HTTP_Request  *request,
                             const DHOP_HTTP_Response *response)
{
    int         ret;
    int         len;
    int         i, k;
    int         length;
    char        *cmd;
    char        *buffer;
    char        *outbuf;

    for (i = 0, k = 0; request->header->url[i] != '\0'; i++) {
        if (request->header->url[i] == '/') {
            cmd = request->header->url + i;
        }
        if (request->header->url[i] == '?') {
            break;
        }
    }

    len = 1024;
    buffer = (char*)malloc(len);

    if (0 == strncmp(cmd, "/setConfig?", 11)) {
        memset(buffer, 0, len);

        len /= 2;
        outbuf = buffer + len;

        request->readContent((DH_String)request->token, buffer, (DH_Uint32*)&len);
        len += 1; // 末尾增加'\0'

        app_http_urldecode(buffer, outbuf, len);

        app_config_parse(outbuf, &g_app_config);

        app_config_save(&g_app_config);

        length = sprintf(buffer, "{\"status\":\"OK\"}");

        response->addHeader(request->token, "Content-Type", "application/json");
        response->setCode(request->token, DHOP_HTTP_StatusCode_200_OK);
        response->setContentLength(request->token, length);
        response->writeContent(request->token, buffer, length);
        response->writeEnd(request->token);
    }
    else if(0==strncmp(cmd,"/startCruise?",13)){
        g_app_config.cruise_start = 1;

        length += sprintf(buffer + length, "0],\"status\":\"OK\"}");
        response->addHeader(request->token, "Content-Type", "application/json");
        response->setCode(request->token, DHOP_HTTP_StatusCode_200_OK);
        response->setContentLength(request->token, length);
        response->writeContent(request->token, buffer, length);
        response->writeEnd(request->token);
    }
    else if(0==strncmp(cmd,"/stopCruise?",12)){
        g_app_config.cruise_start = 0;
        
        length += sprintf(buffer + length, "0],\"status\":\"OK\"}");
        response->addHeader(request->token, "Content-Type", "application/json");
        response->setCode(request->token, DHOP_HTTP_StatusCode_200_OK);
        response->setContentLength(request->token, length);
        response->writeContent(request->token, buffer, length);
        response->writeEnd(request->token);
    }
    else if(0==strncmp(cmd,"/set_Home?",10)){
        DH_Int32 ret = -1;
        typedef int (*PTZ_setHome)(DH_Handle);
        PTZ_setHome DHOP_PTZ_setHomeFunc = (PTZ_setHome)GetFunc("DHOP_PTZ_setHome");
        ret = DHOP_PTZ_setHomeFunc(g_app_global.hPTZ);
        if(DHOP_SUCCESS != ret){
            DHOP_LOG_ERROR("DHOP_PTZ_setHomeFunc fail with %#x\n",ret);
        }

        length += sprintf(buffer + length, "0],\"status\":\"OK\"}");
        response->addHeader(request->token, "Content-Type", "application/json");
        response->setCode(request->token, DHOP_HTTP_StatusCode_200_OK);
        response->setContentLength(request->token, length);
        response->writeContent(request->token, buffer, length);
        response->writeEnd(request->token);
    }
    return DHOP_SUCCESS;
}


DH_Void app_exit_callback()
{
    DHOP_LOG_INFO("app_size_limit-%d will exit\n", getpid());

    DHOP_HTTP_offline();

    app_net_deinit();

    app_enc_deinit();

    app_yuv_deinit();

    app_ai_deinit();

    object_ot_deinit();

    DHOP_SYS_deInit();

    app_ptz_deinit();
}

