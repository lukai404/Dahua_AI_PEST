
var G_isPlugin = false;
// 向select中添加数据，仅仅是为了方便demo的操作
function addOption (shapeID) {
    var $select = document.getElementById('select_data');
    $select.options.add(new Option(shapeID, shapeID));
}

// 绘制的回调，最新的回调数据都会在这里
var drawCallback = function (arg) {
    if (arg.event.type != "drawFinish") {
        return;
    }

    var shapeID = arg.shapeId;

  //  addOption(shapeID);
}

function bind (sdevInterface) {
    // 无插件不支持开启和关闭流
    //document.getElementById('close_stream').onclick = function () {
    //    sdevInterface.close();
    //};

    // 无插件不支持开启和关闭流
    //document.getElementById('open_stream').onclick = function () {
    //    sdevInterface.open();
    //};

    // 开启ivs显示
    document.getElementById('OpenIvsEnable').onclick = function () {
        sdevInterface.SetIVSEnable(true)
    }
    // 关闭ivs显示
    document.getElementById('CloseIvsEnable').onclick = function () {
        sdevInterface.SetIVSEnable(false);
    }

    //// 绘制垂直线
    //document.getElementById('DrawVertLine').onclick = function () {
    //    var title = document.getElementById('input_title').value;
    //    var shapeID = sdevInterface.DrawVertLine(title);
    //    G_isPlugin && addOption(shapeID);
    //}

    //// 绘制水平线
    //document.getElementById('DrawHorizLine').onclick = function () {
    //    var title = document.getElementById('input_title').value;
    //    var shapeID = sdevInterface.DrawHorizLine(title);
    //    G_isPlugin && addOption(shapeID);
    //}

    //// 绘制直线
    //document.getElementById('DrawLine').onclick = function () {
    //    var title = document.getElementById('input_title').value;
    //    var shapeID = sdevInterface.DrawLine(title);
    //    G_isPlugin && addOption(shapeID);
    //}

    //// 绘制折线
    //document.getElementById('DrawPolyLine').onclick = function () {
    //    var title = document.getElementById('input_title').value;
    //    var shapeID = sdevInterface.DrawPolyLine(title);
    //    G_isPlugin && addOption(shapeID);
    //}

    //// 绘制矩形
    //document.getElementById('DrawRectangle').onclick = function () {
    //    var title = document.getElementById('input_title').value;
    //    var shapeID = sdevInterface.DrawRectangle(title);
    //    G_isPlugin && addOption(shapeID);
    //}

    //// 绘制多边形
    //document.getElementById('DrawPolygon').onclick = function () {
    //    var title = document.getElementById('input_title').value;
    //    var shapeID = sdevInterface.DrawPolygon(title);
    //    G_isPlugin && addOption(shapeID);
    //}

    //// 移除指定id的对象
    //document.getElementById('RemoveObjById').onclick = function () {
    //    var $select = document.getElementById('select_data');
    //    var shapeID = $select.value - 0;

    //    sdevInterface.RemoveObjById(shapeID);

    //    // 下面的只是为了联动界面，可以不管
    //    var optionArr = $select.options;
    //    for (var i = 0, len = optionArr.length; i < len; i++) {
    //        if (optionArr[i].value == shapeID) {
    //            $select.removeChild(optionArr[i]);
    //            break;
    //        }
    //    }
    //}

    //// 移除当前活动对象
    //document.getElementById('RemoveActiveObj').onclick = function () {
    //    var res = sdevInterface.RemoveActiveObj();

    //    var $select = document.getElementById('select_data');

    //    // 下面的只是为了联动界面，可以不管
    //    var optionArr = $select.options;
    //    for (var i = 0; i < optionArr.length; i++) {
    //        for (var j = 0, jLen = res.length; j < jLen; j++) {
    //            if (optionArr[i].value == res[j]) {
    //                $select.removeChild(optionArr[i]);
    //            }
    //        }
    //    }
    //}

    //// 设置当前id对象为活动对象
    //document.getElementById('SetActiveObj').onclick = function () {
    //    var shapeId = document.getElementById('select_data').value;
    //    sdevInterface.SetActiveObj(shapeId);
    //}

    //// 为指定id对象设置标题
    //document.getElementById('SetText').onclick = function () {

    //    var title = document.getElementById('input_SetText').value;
    //    var shapeId = document.getElementById('select_data').value;
    //    sdevInterface.SetText(shapeId, title);
    //}

    //// 获取当前活动对象
    //document.getElementById('GetActiveObject').onclick = function () {
    //    var res = sdevInterface.GetActiveObject(); // 返回的数据对象
    //    document.getElementById('text_btn_GetActiveObjType').innerHTML = JSON.stringify(res, null, 2);
    //}

    //// 无插件下，是退出当前绘制状态，插件下，是隐藏当前绘制内容
    //document.getElementById('removeShapeDrawEvent').onclick = function () {
    //    sdevInterface.removeShapeDrawEvent();
    //}
}

var sdevCanvasWrap = document.getElementById('sdev_stream_wrap'); // 放置视频元素的容器
var sdev = new SDEV(sdevCanvasWrap, 640, 480);

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
	bind(sdev); // 登录成功后绑定绘图事件

    if (res.code == 0x10000403) {
        console.log('socket链接错误');
    }
	document.getElementById('top_tip').innerHTML = '';
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