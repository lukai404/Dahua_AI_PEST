#ifndef _TOOLS_H
#define _TOOLS_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "dhop_sys.h"
#include "dhop_app.h"
#include "dhop_version.h"
#include "dhop_http.h"
#include "dhop_yuv.h"
#include "dhop_codec.h"
#include "dhop_mem.h"
#include "dhop_log.h"
#include "dhop_event_common.h"
#include "dhop_event_pushCustom.h"
#include "dhop_venc.h"
#include "config.h"
#include "dhop_osd.h"
#include "dhop_ai.h"
#include "dhop_ptz.h"
#include "object_ot.h"
#include "yolo_object.h"
#include <dlfcn.h>
#include <math.h>

#define ALIGN(value, align)   ((( (value) + ( (align) - 1 ) ) \
            / (align) ) * (align) )
#define FLOOR(value, align)   (( (value) / (align) ) * (align) )

#define APP_MAX_AI_RESULT_NUM   (64)
#define POSITION_NUMBER 251

typedef struct{
    DH_Int32 position;
    DH_Int32 pest_num;
    DH_Int32 pic_size;
    DH_Int32 stop;
    char    time[20];
    obj_info_t bboxes[APP_MAX_AI_RESULT_NUM];
}send_infos;

struct app_global_t {
    int                         hNet;
    DH_Handle                   hYuv;
    DH_Handle                   hVenc;
    DH_Handle                   hPTZ;
    DHOP_AI_NNX_Handle          hNNX;

    DH_Int32                    resultNum;
    DH_Int32                    encAlignW;
    DH_Int32                    encAlignH;
    obj_info_t                  aiResult[APP_MAX_AI_RESULT_NUM];
    DHOP_PTZ_Space              positions_infos[POSITION_NUMBER];
};

struct paint_info_t {
    DHOP_OSD_Element            element[2];
    DHOP_OSD_Polygon            rect;
    DHOP_OSD_Point              points[4];
    DHOP_OSD_Text               text;
    DH_Char                     szText[24];
};

extern struct app_config        g_app_config;
extern struct app_global_t      g_app_global;


extern int app_net_init();
extern int app_net_deinit();
extern int app_net_reinit();
extern int app_enc_init();
extern int app_enc_deinit();
extern int app_yuv_init();
extern int app_yuv_deinit();
extern int app_ptz_init();
extern int app_ptz_deinit();
extern int app_ai_init();
extern int app_ai_deinit();


extern int app_http_urldecode(const char* src, char* outbuf, int size);
extern int app_http_on_request(const DHOP_HTTP_Request  *request,
                             const DHOP_HTTP_Response *response);
extern void app_exit_callback();
extern void* GetFunc(const char* FuncName);
extern int app_ai_process(DHOP_AI_NNX_Handle hNNX, DHOP_YUV_FrameData2 * frame, send_infos* results);
extern int app_result_snap(send_infos* result, DHOP_YUV_FrameData2* frame);
extern int app_size_limit(DH_Int32 pix, DH_Int32 max);

#endif