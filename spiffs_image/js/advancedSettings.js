
var TABLENAME = "settingsTable";

function initSettings() {
	getSettings( "getAdvSettings" ,TABLENAME);
}

function setAdvUserDefaults(){
	getItem("setAdvUserDefaults");
}

function forgetWifi() {
	getItem("forgetWifi");
}

function checkUpdates () {
	getItem( "checkUpdates");
}


