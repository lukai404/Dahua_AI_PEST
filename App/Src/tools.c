#include "tools.h"

struct app_config        g_app_config;
struct app_global_t      g_app_global;

void *GetFunc(const char *funcName) {
    static void *handle = NULL; 

    if (handle == NULL) {
        handle = dlopen("/libsoc/libitop_sdk.so", RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "Error: %s\n", dlerror());
            return NULL;
        }
    }
    // 获取函数指针
    void *funcPtr = dlsym(handle, funcName);
    if (!funcPtr) {
        DHOP_LOG_ERROR("fail to get funcPtr %s\n",funcName);
        fprintf(stderr, "Error: %s\n", dlerror());
        return NULL;
    }
    return funcPtr;
}


DH_Int32 setHOME(){
    DH_Int32 ret = -1;
    DH_Int32 (*DHOP_PTZ_setHome)(DH_Handle phPtz);
    DHOP_PTZ_setHome = (DH_Int32 (*)(DH_Handle))GetFunc("DHOP_PTZ_setHome");
    DHOP_LOG_INFO("before DHOP_PTZ_setHome\n");
    ret = DHOP_PTZ_setHome(g_app_global.hPTZ);
    DHOP_LOG_INFO("after DHOP_PTZ_setHome\n");
    return ret;
}

DH_Int32 app_ptz_init(){
    typedef int (*PTZ_getPtzChnNum)();
    typedef int (*PTZ_getChnCaps)(int,DHOP_PTZ_Caps*);
    typedef int (*PTZ_open)(DHOP_PTZ_OpenParam*,DH_Handle*);
    DHOP_PTZ_Caps caps;
    DH_Int32 ret = -1;
    DHOP_PTZ_OpenParam pParm;
    pParm.channel = 0;
    PTZ_getPtzChnNum DHOP_PTZ_getPtzChnNumFunc = (PTZ_getPtzChnNum)GetFunc("DHOP_PTZ_getPtzChnNum");
    PTZ_getChnCaps DHOP_PTZ_getChnCapsFunc = (PTZ_getChnCaps)GetFunc("DHOP_PTZ_getChnCaps");
    PTZ_open       DHOP_PTZ_openFunc = (PTZ_open)GetFunc("DHOP_PTZ_open");
    DHOP_LOG_INFO("before Func\n");
    int num = DHOP_PTZ_getPtzChnNumFunc();
    if(num < 0){
        DHOP_LOG_ERROR("DHOP_PTZ_getPtzChnNumFunc fail\n");
    }
    ret = DHOP_PTZ_openFunc(&pParm,&(g_app_global.hPTZ));
    if(DHOP_SUCCESS!=ret){
        DHOP_LOG_ERROR("DHOP_PTZ_openFunc fail!\n");
    }
    DHOP_LOG_INFO("app_ptz_init succes\n");
    return ret;
}

DH_Int32 app_ptz_deinit(){
    typedef int (*PTZ_close)(DH_Handle*);
    PTZ_close DHOP_PTZ_closeFunc = (PTZ_close)GetFunc("DHOP_PTZ_close");
    DH_Int32 ret = -1;
    DHOP_LOG_INFO("before DHOP_PTZ_closeFunc\n");
    ret = DHOP_PTZ_closeFunc(&(g_app_global.hPTZ));
    if(ret!=DHOP_SUCCESS){
        DHOP_LOG_ERROR("DHOP_PTZ_closeFunc fail\n");
    }
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

static DH_Uint32 DHOP_SMP_YUV_getDataSize(DH_Uint16 stride, DH_Uint16 height, DH_Uint16 format)
{
    DH_Uint32 dataSize = 0;

    switch(format)
    {
        case DHOP_YUV_FMT_ONLY_Y:
             dataSize = stride * height;
        case DHOP_YUV_FMT_420P_I420:
        case DHOP_YUV_FMT_420P_YV12:
        case DHOP_YUV_FMT_420SP_UV:
        case DHOP_YUV_FMT_420SP_VU:
             dataSize = stride * height * 3 >> 1;
        default:
             break;
    }
    return dataSize;
}

// 将检测的结果发送给服务器端
int app_result_send(DHOP_VENC_Result* image, send_infos* result) {
    DH_Int32                ret = -1;

    if (g_app_global.hNet > 0) {
        result->pic_size = image->size;
        ret = send(g_app_global.hNet, result, sizeof(send_infos), 0);
        if (ret < 0) {
            perror("send head failed:");
            app_net_reinit();
            return -2;
        }
        DHOP_LOG_INFO("send reuslt success\n");
        DHOP_LOG_INFO("send size is %d\n",sizeof(send_infos));
        DHOP_LOG_INFO("position is %d\n",result->position);
        DHOP_LOG_INFO("pest num is %d\n",result->pest_num);
        ret = send(g_app_global.hNet, image->virAddr, image->size, 0);
        if (ret < 0) {
            perror("send image failed:");
            app_net_reinit();
            return -3;
        }
        DHOP_LOG_INFO("image send success\n");
        DHOP_LOG_INFO("image size is %d\n",image->size);
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
    DHOP_LOG_INFO("before encReq.region.rb.x\n");
    encReq.region.rb.x  = frame->data.width;
    encReq.region.rb.y  = frame->data.height;
    encReq.data         = frame;
    encReq.timeout      = 200;
    DHOP_LOG_INFO("before DHOP_VENC_sendRequest\n");
    ret = DHOP_VENC_sendRequest(g_app_global.hVenc, &encReq);
    if (ret != DHOP_SUCCESS) {
        DHOP_LOG_WARN("Send enc reqeust failed with %#s\n",ret);
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

/**
 * 将2560*1440的YUV帧，切割成18张512*512的子帧
 * **/
DH_Int32 split_frame(DHOP_YUV_FrameData2* yuvFrame, DHOP_YUV_FrameData2* subFrames,
                        DH_Uint8** pPhyAddrs,DH_Uint8** pVirAddrs)
{
    DH_Int32 ret = -1;
    DHOP_YUV_CopyReq2 copyReq;
    DH_Uint32 dataSize = 0;

    for(int k = 0; k < SUB_FRAME_NUM; k++){
        memset(&copyReq,0,sizeof(copyReq));
        copyReq.cbSize = sizeof(copyReq);
        memcpy(&copyReq.source,&(yuvFrame->data),sizeof(DHOP_YUV_PixelData));
        
        int row = k / 6;
        int col = k % 6;
    
        copyReq.region.lt.x = col==5 ? 2048 : col * 492;
        copyReq.region.lt.y = row==2 ? 928 : row * 492;
        copyReq.region.rb.x = copyReq.region.lt.x + 512;
        copyReq.region.rb.y = copyReq.region.lt.y + 512;

        copyReq.position.x = 0;
        copyReq.position.y = 0;

        copyReq.dest.format = yuvFrame->data.format;
        copyReq.dest.width = 512;
        copyReq.dest.height = 512;
        
        copyReq.dest.stride[0] = ALIGN(copyReq.dest.width, 16);
        copyReq.dest.stride[1] = copyReq.dest.stride[0];
        copyReq.dest.stride[2] = copyReq.dest.stride[1];

        DHOP_LOG_INFO("yuvFrame width: %d,yuvFrame height: %d\n",yuvFrame->data.width,yuvFrame->data.height);
        DHOP_LOG_INFO("yuvFrame format is %u\n",yuvFrame->data.format);
        DHOP_LOG_INFO("copyReq region,lt.x:%d ,lt.y:%d, rb.x:%d, rb.y:%d",copyReq.region.lt.x,copyReq.region.lt.y,copyReq.region.rb.x,copyReq.region.rb.y);
        DHOP_LOG_INFO("copyReq.dest.strides,stride0:%d,stride1:%d,stride2:%d",copyReq.dest.stride[0],copyReq.dest.stride[1],copyReq.dest.stride[2]);

        dataSize = DHOP_SMP_YUV_getDataSize(copyReq.dest.stride[0], copyReq.dest.height, copyReq.dest.format);
       // DHOP_LOG_INFO("DHOP_SMP_YUV_getDataSize success\n");
        ret = DHOP_MEM_blockAlloc(dataSize, 16, DH_FALSE, (DH_Void **)&pPhyAddrs[k], (DH_Void **)&pVirAddrs[k]);
        if(DHOP_SUCCESS != ret)
        {
            DHOP_LOG_ERROR("DHOP_MEM_blockAlloc fail with %#x\n", ret);
            return ret;
        }
        //DHOP_LOG_INFO("DHOP_MEM_blockAlloc success\n");

        copyReq.dest.phyAddr.nv21.y = pPhyAddrs[k];
        copyReq.dest.phyAddr.nv21.vu = pPhyAddrs[k] + copyReq.dest.stride[0] * copyReq.dest.height;
        copyReq.dest.virAddr.nv21.y = pVirAddrs[k];
        copyReq.dest.virAddr.nv21.vu = pVirAddrs[k] + copyReq.dest.stride[0] * copyReq.dest.height;

        ret = DHOP_YUV_copy2(g_app_global.hYuv,&copyReq);
        if(DHOP_SUCCESS != ret){
            DHOP_LOG_ERROR("DHOP_YUV_copy2 fail with %#x\n", ret);
            return ret;
        }
       // DHOP_LOG_INFO("DHOP_YUV_copy2 success\n");

        memset(&subFrames[k],0,sizeof(subFrames[k]));
       // DHOP_LOG_INFO("memset(subFrames[k]) success\n");
        subFrames[k].cbSize = sizeof(DHOP_YUV_FrameData2);
        subFrames[k].yuvStamp = yuvFrame->yuvStamp;
        memcpy(&subFrames[k].data,&copyReq.dest,sizeof(DHOP_YUV_PixelData));
        DHOP_LOG_INFO("subFrame %d copy done!\n",k);

    }
    return 0;
}

//实现subFrame到global的坐标转换
void convert_coordinates(obj_info_t* bboxes,int batchIdx){
    int row = batchIdx / 6;
    int col = batchIdx % 6;
    int start_x = col==5 ? 2048 : col * 492;
    int start_y = row==2 ? 928  : row * 492;
    bboxes->actual.lt.x = start_x + bboxes->actual.lt.x;
    bboxes->actual.lt.y = start_y + bboxes->actual.lt.y;
    bboxes->actual.rb.x = start_x + bboxes->actual.rb.x;
    bboxes->actual.rb.y = start_y + bboxes->actual.rb.y;
    DHOP_LOG_INFO("the %d batchIdx done!\n",batchIdx);
}

// AI的运行
DH_Int32 app_ai_process(DHOP_AI_NNX_Handle hNNX, DHOP_YUV_FrameData2 * frame, send_infos* results)
{
    DH_Int32                ret = -1, i,k,x;
    DH_Ptr                  ptrs[2];
    DH_Ptr                  ptrs_HW[2];
    DH_Int32                strides[2];
    DHOP_AI_IMG_Handle      hImg[18];
    DHOP_YUV_FrameData2     subFrames[18];     
    DH_Uint8*               pPhyAddrs[18];
    DH_Uint8*               pVirAddrs[18];
    DH_Uint32               type = DHOP_AI_NNX_RESULT_TYPE_MAX;
    DHOP_AI_MAT_Handle      yoloMat;
    DH_Int32                h,start;
    DHOP_AI_NNX_ResultYolo *yolo_result;
    DH_String               inputNames[MAX_OUTPUT_NUM];
    DH_Uint32               inputNum = MAX_OUTPUT_NUM;
    DH_String               outputNames[MAX_OUTPUT_NUM];
    DH_Uint32               outputNum = MAX_OUTPUT_NUM;

    //split 的逻辑
    ret = split_frame(frame,subFrames,pPhyAddrs,pVirAddrs);

   // 创建DHOP_AI_IMG_Handle的句柄，需要DHOP_AI_IMG_destroy()来释放img的内存
    for(x = 0 ; x < 18 ; x++ ){
        ptrs[0]    = subFrames[x].data.virAddr.nv21.y;
        ptrs[1]    = subFrames[x].data.virAddr.nv21.vu;
        ptrs_HW[0] = subFrames[x].data.phyAddr.nv21.y;
        ptrs_HW[1] = subFrames[x].data.phyAddr.nv21.vu;
        strides[0] = subFrames[x].data.stride[0];
        strides[1] = subFrames[x].data.width;
        //DHOP_LOG_INFO("strides[0]: %d,strides[1]: %d\n",strides[0],strides[1]);

        ret = DHOP_AI_IMG_create(&(hImg[x]),
                                 512,
                                 512,
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
        
    }
    DHOP_LOG_INFO("DHOP_AI_IMG_create success\n");

    //获取输入blob名称
    ret = DHOP_AI_NNX_getInputBlobNames(g_app_global.hNNX, (DH_Byte**)&inputNames, &inputNum);
    if(ret != DHOP_SUCCESS){
        DHOP_LOG_ERROR("DHOP_AI_NNX_getInputBlobNames fail with %#x\n",ret);
    }
        

    // 设置图片输入
    ret = DHOP_AI_NNX_setInputImg(g_app_global.hNNX, (const DH_String)inputNames[0], hImg, 18);
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("dhop ai set input img fail");
        goto err1;
    }

    DHOP_LOG_INFO("DHOP_AI_NNX_setInputImg success\n");
    // AI运行
    ret = DHOP_AI_NNX_run(g_app_global.hNNX);
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("dhop ai run fail");
        goto err1;
    }
    DHOP_LOG_INFO("DHOP_AI_NNX_run success\n");

    //get output blob name

    ret = DHOP_AI_NNX_getOutputBlobNames(g_app_global.hNNX, (DH_Byte**)&outputNames, &outputNum);
    if(ret != DHOP_SUCCESS){
        DHOP_LOG_ERROR("DHOP_AI_NNX_getOutputBlobNames fail with %#x\n",ret);
    }

    // 获取AI检测结果
    type = DHOP_AI_NNX_RESULT_TYPE_MAX;
    ret = DHOP_AI_NNX_getResult(g_app_global.hNNX, (const DH_String)outputNames[0], &type, &yoloMat);
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

    k = 0;

    for (i = start; i < start+h; i++)
    {
        // 过滤置信度极小的值
        if ( yolo_result[i].prob > 0.2 )
        {
            if (k < APP_MAX_AI_RESULT_NUM) {
                // 算法输出的是0~1的浮点数据，要转换成YUV frame的宽高坐标
                int ltx = app_size_limit((yolo_result[i].x - yolo_result[i].w/2) * 512, 512);
                int lty = app_size_limit((yolo_result[i].y - yolo_result[i].h/2) * 512, 512);
                int rbx = app_size_limit((yolo_result[i].x + yolo_result[i].w/2) * 512, 512);
                int rby = app_size_limit((yolo_result[i].y + yolo_result[i].h/2) * 512, 512);
                int width = rbx - ltx;
                int height = rby - lty;
                DHOP_LOG_INFO("yolo_result[i].x :%f\n",yolo_result[i].x);
                DHOP_LOG_INFO("yolo_result[i].y :%f\n",yolo_result[i].y);
                DHOP_LOG_INFO("yolo_result[i].w :%f\n",yolo_result[i].w);
                DHOP_LOG_INFO("yolo_result[i].h :%f\n",yolo_result[i].h);
                DHOP_LOG_INFO("ltx :%d\n",ltx);
                DHOP_LOG_INFO("rby :%d\n",rby);
                DHOP_LOG_INFO("pest width: %d ,pest height: %d\n",width,height);
                if(width > 85 || height >85 || ltx < 0 || lty < 0 || rbx > 512 || rby >512){
                    DHOP_LOG_INFO("pass\n");
                    continue;
                }
                results->bboxes[k].actual.lt.x = ltx;
                results->bboxes[k].actual.lt.y = lty;
                results->bboxes[k].actual.rb.x = rbx;
                results->bboxes[k].actual.rb.y  = rby;
                results->bboxes[k].classId = yolo_result[i].classIdx;
                results->bboxes[k].conf = yolo_result[i].prob;
                convert_coordinates(&(results->bboxes[k]),yolo_result[i].batchIdx);
                DHOP_LOG_INFO("convert_coordinates success\n");
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
    for(x=0;x<18;x++){
        ret = DHOP_AI_IMG_destroy(hImg[x]);
        if (DHOP_SUCCESS != ret)
        {
            DHOP_LOG_ERROR("DHOP_AI_IMG_destroy fail\n");
        }
        ret = DHOP_MEM_blockFree(pPhyAddrs[x], pVirAddrs[x]);
        if(DHOP_SUCCESS != ret)
        {
            DHOP_LOG_ERROR("DHOP_MEM_blockFree fail with %#x\n", ret);
        }
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

        length = sprintf(buffer, "{\"status\":\"OK\"}");

        response->addHeader(request->token, "Content-Type", "application/json");
        response->setCode(request->token, DHOP_HTTP_StatusCode_200_OK);
        response->setContentLength(request->token, length);
        response->writeContent(request->token, buffer, length);
        response->writeEnd(request->token);
    }
    else if(0==strncmp(cmd,"/stopCruise?",12)){
        g_app_config.cruise_start = 0;
        
        length = sprintf(buffer, "{\"status\":\"OK\"}");

        response->addHeader(request->token, "Content-Type", "application/json");
        response->setCode(request->token, DHOP_HTTP_StatusCode_200_OK);
        response->setContentLength(request->token, length);
        response->writeContent(request->token, buffer, length);
        response->writeEnd(request->token);
    }
    else if(0==strncmp(cmd,"/set_Home?",10)){
        DH_Int32 ret = -1;
        DHOP_LOG_INFO("click setHome\n");
        ret = setHOME();
        length = sprintf(buffer, "{\"status\":\"OK\"}");

        response->addHeader(request->token, "Content-Type", "application/json");
        response->setCode(request->token, DHOP_HTTP_StatusCode_200_OK);
        response->setContentLength(request->token, length);
        response->writeContent(request->token, buffer, length);
        response->writeEnd(request->token);
        DHOP_LOG_INFO("after response\n");
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

    app_ptz_deinit();

    DHOP_SYS_deInit();
    
}

