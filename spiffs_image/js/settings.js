
var TABLENAME = "settingsTable";

function initSettings() {
	getSettings( "getSettings" ,TABLENAME);
}

function setuserDefaults(){
	getIntem("setUserDefaults");
	getSettings( "getSettings" ,TABLENAME);
}


