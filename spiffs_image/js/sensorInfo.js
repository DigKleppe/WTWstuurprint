
var firstTime = true;
var INFOTABLENAME = "sensorInfoTable";

function init() {
	setInterval(function() { timer() }, 1000);
}

function timer() {
	if (document.visibilityState == "hidden")
		return;
	getInfo( "getSensorInfo", INFOTABLENAME, firstTime);
	firstTime = false;
}

