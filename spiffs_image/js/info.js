

var INFOTABLENAME = "infoTable";
var firstTime = true;

function init() {
	setInterval(function() { timer() }, 1000);
}

function forgetWifi() {
	getItem("forgetWifi");
}

function checkUpdates () {
	getItem( "checkUpdates");
}

function timer() {
	if (document.visibilityState == "hidden")
		return;
	getInfo( "getCommonInfo", INFOTABLENAME, firstTime);
	firstTime = false;
}

