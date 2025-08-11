var INFOTABLENAME = "infoTable";
var firstTime = true;

function init() {
	setInterval(function() { timer() }, 1000);
}

function timer() {
	if (document.visibilityState == "hidden")
		return;
	getInfo( "getFanInfo", INFOTABLENAME, firstTime);
	firstTime = false;
}


function resetFanLimits () {
	getItem ( "resetFanLimits");
}