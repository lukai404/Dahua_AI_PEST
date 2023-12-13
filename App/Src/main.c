#include "tools.h"


#define ALIGN(value, align)   ((( (value) + ( (align) - 1 ) ) \
            / (align) ) * (align) )
#define FLOOR(value, align)   (( (value) / (align) ) * (align) )


extern struct app_config        g_app_config;
extern struct app_global_t      g_app_global;

DH_Int32 Listnum_b5[33] = { 16,3,9,3,13,3,9,3,15,3,9,3,13,3,9,3,17,3,9,3,13,3,9,3,15,3,9,3,13,3,9,3,16 };

DH_Int32 moveToHOME(){
    DH_Int32 ret = -1;
    typedef int (*PTZ_gotoHome)(DH_Handle);
    PTZ_gotoHome DHOP_PTZ_gotoHomeFunc = (PTZ_gotoHome)GetFunc("DHOP_PTZ_gotoHome");
    ret = DHOP_PTZ_gotoHomeFunc(g_app_global.hPTZ);
    return ret;
}


DH_Int32 moveToPTZ(DHOP_PTZ_Space* pos){
    DH_Int32 ret = -1;
    typedef int (*PTZ_absoluteMoveFunc)(DH_Handle, DHOP_PTZ_Space*, DHOP_PTZ_Speed*);
    DHOP_PTZ_Speed speed;
    speed.fSpeedX = 1;
    speed.fSpeedY = 1;
    speed.fZoom = 1;

    PTZ_absoluteMoveFunc DHOP_PTZ_absoluteMoveFunc = (PTZ_absoluteMoveFunc)GetFunc("DHOP_PTZ_absoluteMove");
    ret = DHOP_PTZ_absoluteMoveFunc(g_app_global.hPTZ,pos,&speed);
    return ret;
}


DH_Int32 getPTZStatus(DHOP_PTZ_Status* positions){
    DH_Int32 ret = -1;
    typedef int (*PTZ_getStatus)(DH_Handle,DHOP_PTZ_Status*);
    PTZ_getStatus DHOP_PTZ_getStatusFunc = (PTZ_getStatus)GetFunc("DHOP_PTZ_getStatus");
    ret = DHOP_PTZ_getStatusFunc(g_app_global.hPTZ,positions);
    return ret;
}


DH_Int32 compute_position(double h){
    int count = 0;
    int NumberofCircle = 33,NumberofPerCircle = 17;
    int ret = -1;                        
    float gap = 5625;
    DH_Int32 Temp_P,Temp_T,Start_P;
    DH_Uint32 Temp_Z;
    DHOP_PTZ_Status home_position;

    ret = getPTZStatus(&home_position); 
    if(DHOP_SUCCESS!=ret){
        DHOP_LOG_ERROR("getPTZStatus fail with %#x\n",ret);
        return ret;
    }
    Start_P = home_position.postion->nPositionX - 9000;
    if(Start_P < 0) Start_P = 36000 + Start_P;

    for(int N=0;N<NumberofCircle;N++){
        Temp_P = Start_P + N*gap;
        if(Temp_P > 36000) Temp_P = Temp_P - 36000;
        for(int col = NumberofPerCircle - Listnum_b5[N];col < NumberofPerCircle;col++){
            g_app_global.positions_infos[count].nPositionX = Temp_P;
            if(col==0){
                Temp_T = 90000;
            }
            else{
                double distance = (double)col * 0.185;  //0.185为B5纸张高度
                double act = atan(h/distance);
                Temp_T = (DH_Int32)((180 /3.14159f)*act*100);
            }
            g_app_global.positions_infos[count].nPositionY = Temp_T;
            g_app_global.positions_infos[count].nZoom = 1000;
            count++;
        }
    }
    return 1;
}

void cruise_action(){
    DH_Int32 ret = -1;
    DHOP_YUV_FrameData2     yuvFrame;
    //goto HOME
    ret = moveToHOME();
    if(DHOP_SUCCESS!=ret){
        DHOP_LOG_ERROR("moveToHOME failed with %#x\n",ret);
    }
    sleep(3);
    ret = compute_position(1.7);
    if(DHOP_SUCCESS!=ret){
        DHOP_LOG_ERROR("compute_position failed with %#x\n",ret);
    }
    
    //for cruise
    for(int pos = 0;pos < POSITION_NUMBER; pos++){
        if(g_app_config.cruise_start==0){
            break;
        }
        //go to PTZ
        ret = moveToPTZ(&(g_app_global.positions_infos[pos]));
        if(DHOP_SUCCESS!=ret){
            DHOP_LOG_ERROR("moveToPTZ fail with %#x\n",ret);
        }
        sleep(3);
        memset(&yuvFrame, 0, sizeof(yuvFrame));

        // 获取YUV通道数据
        ret = DHOP_YUV_getFrame2(g_app_global.hYuv, &yuvFrame, 2000);
        if(DHOP_SUCCESS!=ret){
            DHOP_LOG_ERROR("DHOP_YUV_getFrame2 fail with %#x\n",ret);
        }

        send_infos results;
        results.position = pos;
        ret = app_ai_process(g_app_global.hNNX,&yuvFrame,&results);

        //将frame编码,并同结果一起发送给桌面端
        ret = app_result_snap(&results, &yuvFrame);

        ret = DHOP_YUV_releaseFrame2(g_app_global.hYuv, &yuvFrame);
        if(DHOP_SUCCESS != ret) {
            DHOP_LOG_ERROR("Release YUV frame data fail with %#x\n", ret);
        }
    }
}


void Inference_benchmark(){
    char jpg_name[30];
    DHOP_AI_IMG_Handle image;
    DH_Int32 ret = -1;
    int MAX_OUTPUT_NUM = 15;
    send_infos results;
    ret = app_net_reinit();
    if(DHOP_SUCCESS !=ret){
        DHOP_LOG_ERROR("app net reinit failed with %#x\n",ret);
    }
    for(int step = 0; step < 18 ; step++){
        if(!g_app_config.cruise_start){
            goto err0;
        }
        sprintf(jpg_name,"./model/test/202312111705_Preset%d.jpg",step);
        // creat DHOP_AI_IMG_Handle. need use DHOP_AI_IMG_destroy() to release img mem
        DHOP_AI_IMG_Handle hImg;
        ret = DHOP_AI_IMGUTILS_createFromFile(&hImg, jpg_name , DHOP_AI_IMG_CS_YUV420SP_VU);
        if(ret != DHOP_SUCCESS){
            DHOP_LOG_ERROR("DHOP_AI_IMGUTILS_createFromFile fail with %#x\n",ret);
        }

        //get input blob name
        DH_String inputNames[MAX_OUTPUT_NUM];
        DH_Uint32 inputNum = MAX_OUTPUT_NUM;
        ret = DHOP_AI_NNX_getInputBlobNames(g_app_global.hNNX, (DH_Byte**)&inputNames, &inputNum);
        if(ret != DHOP_SUCCESS){
            DHOP_LOG_ERROR("DHOP_AI_NNX_getInputBlobNames fail with %#x\n",ret);
        }
        
        // set input image
        ret = DHOP_AI_NNX_setInputImg(g_app_global.hNNX , (const DH_String)inputNames[0], &hImg, 1);
        if(ret != DHOP_SUCCESS){
            DHOP_LOG_ERROR("DHOP_AI_NNX_setInputImg fail with %#x\n",ret);
        }

        // run engine
        ret = DHOP_AI_NNX_run(g_app_global.hNNX);
        if(ret != DHOP_SUCCESS){
            DHOP_LOG_ERROR("DHOP_AI_NNX_run fail with %#x\n",ret);
        }

        //get output blob name
        DH_String outputNames[MAX_OUTPUT_NUM];
        DH_Uint32 outputNum = MAX_OUTPUT_NUM;
        ret = DHOP_AI_NNX_getOutputBlobNames(g_app_global.hNNX, (DH_Byte**)&outputNames, &outputNum);
        if(ret != DHOP_SUCCESS){
            DHOP_LOG_ERROR("DHOP_AI_NNX_getOutputBlobNames fail with %#x\n",ret);
        }
        
        // get result
        DH_Uint32 type = DHOP_AI_NNX_RESULT_TYPE_MAX; 
        DHOP_AI_MAT_Handle yoloMat;
        ret = DHOP_AI_NNX_getResult(g_app_global.hNNX , (const DH_String)outputNames[0], &type, &yoloMat);
        if(ret != DHOP_SUCCESS){
            DHOP_LOG_ERROR("DHOP_AI_NNX_getResult fail with %#x\n",ret);
        }

        DH_Int32 h,start;
        ret = DHOP_AI_MAT_getActiveRange(yoloMat, 0, &start, &h); 
        if(ret != DHOP_SUCCESS){
            DHOP_LOG_ERROR("DHOP_AI_MAT_getActiveRange fail with %#x\n",ret);
        }
        
        DHOP_AI_NNX_ResultYolo *yolo_result = (DHOP_AI_NNX_ResultYolo *)DHOP_AI_MAT_ptr2(yoloMat, NULL);
        memset(&results,0,sizeof(results));
        results.position = step;
        int k = 0;
        for(int i = start; i < start+h; i++ )
        {
            if (k < APP_MAX_AI_RESULT_NUM) {
                // 算法输出的是0~1的浮点数据，要转换成YUV frame的宽高坐标
                results.bboxes[k].actual.lt.x = app_size_limit((yolo_result[i].x - yolo_result[i].w/2) * 512 ,512);
                results.bboxes[k].actual.lt.y = app_size_limit((yolo_result[i].y - yolo_result[i].h/2) * 512, 512);
                results.bboxes[k].actual.rb.x = app_size_limit((yolo_result[i].x + yolo_result[i].w/2) * 512, 512);
                results.bboxes[k].actual.rb.y  = app_size_limit((yolo_result[i].y + yolo_result[i].h/2) * 512, 512);
                results.bboxes[k].conf = yolo_result[i].prob;
                results.bboxes[k].classId = yolo_result[i].classIdx;
                k++;
            }
            else {
                break;
            }
        }
        results.pest_num = k;
        if (g_app_global.hNet > 0) {
            ret = send(g_app_global.hNet, &results, sizeof(results), 0);
            DHOP_LOG_INFO("id:%d\n",step);
            DHOP_LOG_INFO("pest_num is %d\n",results.pest_num);
            DHOP_LOG_INFO("sendinfos size:%d\n",sizeof(results));
            DHOP_LOG_INFO("sendinfos.bboxes size:%d\n",sizeof(results.bboxes));
            if (ret < 0) {
                perror("send head failed:");
                app_net_reinit();
                DHOP_LOG_ERROR("app_net_reinit fail\n");
                return;
            }
        }   
        // destroy DHOP_AI_IMG_Handle
        ret = DHOP_AI_IMGUTILS_destroy(hImg);
        if(ret != DHOP_SUCCESS){
            DHOP_LOG_ERROR("DHOP_AI_IMGUTILS_destroy fail with %#x\n",ret);
        }
    }
err0:
    memset(&results,0,sizeof(results));
    results.stop = 1;
    ret = send(g_app_global.hNet,&results,sizeof(results),0);
    g_app_config.cruise_start = 0;
    if (ret < 0) {
        perror("send head failed:");
        app_net_reinit();
        DHOP_LOG_ERROR("app_net_reinit fail\n");
        return;
    }  
    return;
}

DH_Int32 app_ai_task()
{
    while(1){
        if(g_app_config.cruise_start){
            Inference_benchmark();            
        }
        sleep(1);
    }
}


int main(int argc, char **argv)
{
    DH_Int32                    ret = -1;
    DH_Char                     webUrl[32] = "index.html";
    DHOP_SYS_InitParam          initParam;
    DHOP_APP_ConfigParam        appCfg;
    DHOP_HTTP_AppDefinition     webApp;

    // step1: dhop sys init. regist exit callback func
    memset(&initParam, 0, sizeof(initParam));
    initParam.onExitCallback = app_exit_callback;
    initParam.version = DHOP_SDK_VERSION;
    ret = DHOP_SYS_init(&initParam);
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("DHOP_SYS_init fail with %#x\n", ret);
        return DHOP_FAILED;
    }
    DHOP_LOG_INFO("DHOP_SYS_init suscess!\n");

    /// 忽略SIGPIPE
    signal(SIGPIPE, SIG_IGN);
    
    ret = app_config_init(&g_app_config);
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("app_config_init fail with %#x\n", ret);
        goto err1;
    }
    g_app_config.mask = _CCM_BIT_NONE;

    DHOP_LOG_setLevel((DHOP_LOG_Level)g_app_config.log_level, DHOP_LOG_DEST_WEB);
    DHOP_LOG_setLevel((DHOP_LOG_Level)g_app_config.log_level, DHOP_LOG_DEST_TTY);
    DHOP_LOG_setLevel((DHOP_LOG_Level)g_app_config.log_level, DHOP_LOG_DEST_FILE);

    object_ot_init();

    DHOP_LOG_INFO("enter app_ai_init\n");
    ret = app_ai_init();
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("app_ai_init fail with %#x\n", ret);
        goto err1;
    }

    ret = app_yuv_init();
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("app_yuv_init fail with %#x\n", ret);
        goto err2;
    }

    ret = app_enc_init();
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("app_enc_init fail with %#x\n", ret);
        goto err3;
    }

    ret = app_net_init();
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("app_net_init fail with %#x\n", ret);
    }
    
    memset(&appCfg, 0, sizeof(appCfg));
    appCfg.weburl = webUrl;
    appCfg.urllen = strlen(webUrl);
    DHOP_APP_setConfig(&appCfg);

    memset(&webApp, 0, sizeof(webApp));
    webApp.cbSize = sizeof(webApp);
    webApp.servlet = app_http_on_request;
    ret = DHOP_HTTP_online(&webApp);
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("DHOP_HTTP_online fail with %#x\n", ret);
        goto err5;
    }

    ret = app_ptz_init();
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("app_ptz_init fail with %#x\n", ret);
        goto err6;
    }

    ret = app_ai_task();
    if (DHOP_SUCCESS != ret)
    {
        DHOP_LOG_ERROR("app_ai_task fail with %#x\n", ret);
        goto err7;
    }
    else
    {
        DHOP_LOG_INFO("app_ai_task suscess!\n");
    }

    return 0;

err7:
    app_ptz_deinit();
err6:
    DHOP_HTTP_offline();
err5:
    app_net_deinit();
err4:
    app_enc_deinit();
err3:
    app_yuv_deinit();
err2:
    app_ai_deinit();
err1:
    object_ot_deinit();
    DHOP_SYS_deInit();

    return -1;

}

