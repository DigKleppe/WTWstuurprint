
var TABLENAME = "settingsTable";

function initSettings() {
	getSettings( "getAdvSettings" ,TABLENAME);
}

function setuserDefaults(){
	getItem("setAdvDefaults");
	getSettings( "getAdvSettings" ,TABLENAME);
}

function forgetWifi() {
	getItem("forgetWifi");
}

function checkUpdates () {
	getItem( "checkUpdates");
}


