
var sdevCanvasWrap = document.getElementById('sdev_stream_wrap'); // 放置视频元素的容器
var sdev = new SDEV(sdevCanvasWrap, 540, 380);

var min_rect_name = "最小目标尺寸";
var det_rect_name = "目标检测区"

var min_rect_shape_id = 0;
var det_rect_shape_id = 0;
var min_rect_array = new Array();
var det_rect_array = new Array();

/*
* 方法调用的js
*/
var G_isPlugin = false;


// 绘制的回调，最新的回调数据都会在这里
var drawCallback = function (arg) {
    console.log(arg.event.type + "-->" + arg.shapeId + "-->" + arg.option.ruleName);

    if (arg.event.type == "drawFinish") {

        if (arg.option.ruleName == min_rect_name) {
            min_rect_shape_id = arg.shapeId;

            min_rect_array.push(arg.shapeId);

            document.getElementById('min_region').value = JSON.stringify(arg.data, null, 0);

            var min_w = 0;
            var min_h = 0;
            if (arg.data[0][0] > arg.data[1][0]) {
                min_w = arg.data[0][0] - arg.data[1][0];
            }
            else {
                min_w = arg.data[1][0] - arg.data[0][0];
            }
            if (arg.data[0][1] > arg.data[1][1]) {
                min_h = arg.data[0][1] - arg.data[1][1];
            }
            else {
                min_h = arg.data[1][1] - arg.data[0][1];
            }
            document.getElementById('min_size').value = "width<" + min_w + ", heigth<" + min_h;
        }

        if (arg.option.ruleName == det_rect_name) {
            det_rect_shape_id = arg.shapeId;

            det_rect_array.push(arg.shapeId);

            document.getElementById('detect_region').value = JSON.stringify(arg.data, null, 0);

        }
    }

    if (arg.event.type == "moveFinish") {
        if (arg.option.ruleName == det_rect_name) {
            document.getElementById('detect_region').value = JSON.stringify(arg.data, null, 0);
        }
        if (arg.option.ruleName == min_rect_name) {
            document.getElementById('min_region').value = JSON.stringify(arg.data, null, 0);

            var min_w = 0;
            var min_h = 0;
            if (arg.data[0][0] > arg.data[1][0]) {
                min_w = arg.data[0][0] - arg.data[1][0];
            }
            else {
                min_w = arg.data[1][0] - arg.data[0][0];
            }
            if (arg.data[0][1] > arg.data[1][1]) {
                min_h = arg.data[0][1] - arg.data[1][1];
            }
            else {
                min_h = arg.data[1][1] - arg.data[0][1];
            }
            document.getElementById('min_size').value = "width<" + min_w + ", heigth<" + min_h;
        }
    }
    if (arg.event.type == "selectedFinish") {
        if (arg.option.ruleName == det_rect_name) {
            document.getElementById('detect_region').value = JSON.stringify(arg.data, null, 0);
        }
        if (arg.option.ruleName == min_rect_name) {
            document.getElementById('min_region').value = JSON.stringify(arg.data, null, 0);

            var min_w = 0;
            var min_h = 0;
            if (arg.data[0][0] > arg.data[1][0]) {
                min_w = arg.data[0][0] - arg.data[1][0];
            }
            else {
                min_w = arg.data[1][0] - arg.data[0][0];
            }
            if (arg.data[0][1] > arg.data[1][1]) {
                min_h = arg.data[0][1] - arg.data[1][1];
            }
            else {
                min_h = arg.data[1][1] - arg.data[0][1];
            }
            document.getElementById('min_size').value = "width<" + min_w + ", heigth<" + min_h;
        }
    }
    if (arg.event.type == "resizeFinish") {
        if (arg.option.ruleName == det_rect_name) {
            document.getElementById('detect_region').value = JSON.stringify(arg.data, null, 0);
        }
        if (arg.option.ruleName == min_rect_name) {
            document.getElementById('min_region').value = JSON.stringify(arg.data, null, 0);

            var min_w = 0;
            var min_h = 0;
            if (arg.data[0][0] > arg.data[1][0]) {
                min_w = arg.data[0][0] - arg.data[1][0];
            }
            else {
                min_w = arg.data[1][0] - arg.data[0][0];
            }
            if (arg.data[0][1] > arg.data[1][1]) {
                min_h = arg.data[0][1] - arg.data[1][1];
            }
            else {
                min_h = arg.data[1][1] - arg.data[0][1];
            }
            document.getElementById('min_size').value = "width<" + min_w + ", heigth<" + min_h;
        }
    }
    if (arg.event.type == "addFinish") {
        if (arg.option.ruleName == min_rect_name) {
            min_rect_shape_id = arg.shapeId;
            min_rect_array.push(arg.shapeId);
        }
        if (arg.option.ruleName == det_rect_name) {
            det_rect_shape_id = arg.shapeId;
            det_rect_array.push(arg.shapeId);
        }
    }
    sdev.removeShapeDrawEvent();
    $("#min_region_btn").removeClass("draw_btn_selected");
    $("#detect_region_btn").removeClass("draw_btn_selected");
}

sdev.load(); // 加载所有预处理文件
sdev.on('load', function (res) {
    if (res.code == 0x20000402) {
        // 当前没有安装控件, 执行debugPlugin可以进行本地调试
    }

    var loginOption = {
        ip: location.host
    }
    if (!sdev.isH5Play) {
        G_isPlugin = true;
    }

    sdev.init(loginOption); // 加载完成后，调用init方法
})

sdev.on('init', function (res) {

    if (res.code == 0x10000403) {
        console.log('socket链接错误');
    }
    document.getElementById('top_tip').innerHTML = '';

    DHOP_JS_getCurrentConfig();
})

sdev.on('canvasDraw', function (arg) {
    drawCallback(arg);
})

sdev.on('error', function (res) {
    if (res.code == 101) {
        sdev.close();

        // 码流切换的提示效果
        document.getElementById('top_tip').innerHTML = '码流切换中...'

        // 延迟过大
        sdev.open(1); // 显示辅码流
    }
})


////////////////////////////////////////////

function DHOP_JS_getCurrentConfig() {
    var req_url = window.location.origin + "/DHOP_API/" + getAppName() + "/getConfig" + "?session=" + getSession();

    var l1 = $("#objectTypes");
    l1.empty();
    l1.selectpicker('refresh');

    var l2 = $("#logLevel");

    $.ajax({
        url: req_url,
        type: "post",
        dataType: "json",
        data: null,
        error: function (xhr) {
            // 提交成功后，弹出提示框，并触发1500ms的定时任务
        },
        success: function (response, status, xhr) {
            // 提交成功后，弹出提示框，并触发1500ms的定时任务
            $("#detect_region").val(JSON.stringify(response.det_rect, null, 0));
            DHOP_JS_DrawRegion(2, response.det_rect);
            $("#min_region").val(JSON.stringify(response.min_rect, null, 0));

            var min_w = 0;
            var min_h = 0;
            if (response.min_rect[0][0] > response.min_rect[1][0]) {
                min_w = response.min_rect[0][0] - response.min_rect[1][0];
            }
            else {
                min_w = response.min_rect[1][0] - response.min_rect[0][0];
            }
            if (response.min_rect[0][1] > response.min_rect[1][1]) {
                min_h = response.min_rect[0][1] - response.min_rect[1][1];
            }
            else {
                min_h = response.min_rect[1][1] - response.min_rect[0][1];
            }
            document.getElementById('min_size').value = "width<" + min_w + ", heigth<" + min_h;

            DHOP_JS_DrawRegion(1, response.min_rect);
            $("#event_link_mail").prop("checked", response.mail);
            $("#event_link_snapshot").prop("checked", response.snap);
            $("#event_link_record").prop("checked", response.record);
            $("#ip").val(response.connect.ip);
            $("#port").val(response.connect.port);
            for (i = 0; i < response.obj_maps.length; i++) {
                html = '<option value="' + response.obj_maps[i].id + '">' + response.obj_maps[i].name + '</option>';
                l1.append(html);
            }
            l1.selectpicker('val', response.selected);
            l1.selectpicker('refresh');
            l2.selectpicker('val', response.logLevel);
            l2.selectpicker('refresh');
        }
    });
}

function DHOP_JS_DrawMinRegion() {
    while (true) {
        var el = min_rect_array.pop();
        if (el == undefined) {
            break;
        }
        else {
            sdev.RemoveObjById(el);
        }
    }

    sdev.DrawRectangle(min_rect_name);
}
function DHOP_JS_DrawDetRegion() {
    while (true) {
        var el = det_rect_array.pop();
        if (el == undefined) {
            break;
        }
        else {
            sdev.RemoveObjById(el);
        }
    }

    sdev.DrawRectangle(det_rect_name);
}

function DHOP_JS_AdjustRegion(id) {
    sdev.removeShapeDrawEvent();
    if (id == 1) {
        sdev.SetActiveObj(min_rect_shape_id);
    }
    if (id == 2) {
        sdev.SetActiveObj(det_rect_shape_id);
    }
}

function DHOP_JS_DrawRegion(id, region) {

    if (id == 1) {

        while (true) {
            var el = min_rect_array.pop();
            if (el == undefined) {
                break;
            }
            else {
                sdev.RemoveObjById(el);
            }
        }

        sdev.DrawGraphFromData("Rectangle", region, { ruleName: min_rect_name });
    }

    if (id == 2) {

        while (true) {
            var el = det_rect_array.pop();
            if (el == undefined) {
                break;
            }
            else {
                sdev.RemoveObjById(el);
            }
        }

        sdev.DrawGraphFromData("Rectangle", region, { ruleName: det_rect_name });
    }

}