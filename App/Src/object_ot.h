#ifndef __OBJECT_OT_H__
#define __OBJECT_OT_H__

#include "dhop_type_def.h"

enum {
    _OBJ_STATE_NEW  = 0,
    _OBJ_STATE_UPDATE,
    _OBJ_STATE_HIDE,
    _OBJ_STATE_REMOVE,
};

typedef struct {
    DH_Int32        id;
    DH_Int32        state;
    DH_Rect16       rect;
    DH_Rect16       actual;
    DH_Int32        classId;
} obj_info_t;

extern int object_ot_init();
extern int object_ot_deinit();

// 将新的结果放入
extern int object_ot_update(int num, obj_info_t* list);

// 获取最新的结果
extern int object_ot_result(int* num, obj_info_t* list);

#endif

