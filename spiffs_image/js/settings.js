
var TABLENAME = "settingsTable";

function initSettings() {
	getSettings( "getSettings" ,TABLENAME);
}

function setuserDefaults(){
	getItem("setUserDefaults");
	getSettings( "getSettings" ,TABLENAME);
}


