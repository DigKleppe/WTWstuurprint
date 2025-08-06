
var firstTime = true;
var firstTimeCal = true;

var body;
var infoTbl;
var settingsTbl;
var nameTbl;
var tblBody;
var INFOTABLENAME = "infoTable";
var SETTINGSTABLENAME = "settingsTable";

function makeInfoTable(descriptorData) {
	infoTbl = document.getElementById(INFOTABLENAME);// ocument.createElement("table");
	var x = infoTbl.rows.length
	for (var r = 0; r < x; r++) {
		infoTbl.deleteRow(-1);
	}
	tblBody = document.createElement("tbody");

	var rows = descriptorData.split("\n");

	for (var i = 0; i < rows.length - 1; i++) {
		var row = document.createElement("tr");
		var colls = rows[i].split(",");

		for (var j = 0; j < colls.length; j++) {
			if (i == 0)
				var cell = document.createElement("th");
			else
				var cell = document.createElement("td");

			var cellText = document.createTextNode(colls[j]);
			cell.appendChild(cellText);
			row.appendChild(cell);
		}
		tblBody.appendChild(row);
	}
	infoTbl.appendChild(tblBody);
}

function makeSettingsTable(descriptorData) {

	var colls;
	settingsTbl = document.getElementById(SETTINGSTABLENAME);// ocument.createElement("table");
	var x = settingsTbl.rows.length;
	for (var r = 0; r < x; r++) {
		settingsTbl.deleteRow(-1);
	}
	tblBody = document.createElement("tbody");

	var rows = descriptorData.split("\n");

	for (var i = 0; i < rows.length - 1; i++) {
		var row = document.createElement("tr");
		if (i == 0) {
			colls = rows[i].split(",");
			for (var j = 0; j < colls.length - 1; j++) {
				var cell = document.createElement("th");
				var cellText = document.createTextNode(colls[j]);
				cell.appendChild(cellText);
				row.appendChild(cell);
			}
		}
		else {
			var cell = document.createElement("td");  // name
			var cellText = document.createTextNode(rows[i]);
			cell.appendChild(cellText);
			row.appendChild(cell);

			cell = document.createElement("td");  // value
			var input = document.createElement("input");
			input.setAttribute("type", "number");
			cell.appendChild(input);
			row.appendChild(cell);

			// cell = document.createElement("td");
			// cell.setAttribute("calItem", i);

			var button = document.createElement("button");
			button.innerHTML = "Stel in";
			//	button.className = "button buttonGreen";
			button.className = "button-3";
			cell.appendChild(button);
			cell.setAttribute("settingsItem", i);
			row.appendChild(cell);

			// cell = document.createElement("td");
			// cell.setAttribute("calItem", i);

			// var button = document.createElement("button");
			// button.innerHTML = "Herstel";
			// //	button.className = "button buttonGreen";
			// button.className = "button-3";
			// button.setAttribute("id", "set" + i);

			// cell.appendChild(button);
			// row.appendChild(cell);
		}
		tblBody.appendChild(row);
	}
	settingsTbl.appendChild(tblBody);

	const cells = document.querySelectorAll("td[settingsItem]");
	cells.forEach(cell => {
		cell.addEventListener('click', function() { setCalFunction(cell.closest('tr').rowIndex, cell.cellIndex) });
	});
}

function readInfo(str) {
	makeInfoTable(str);
}

function readSettingsInfo(str) {
	if (SIMULATE) {
		if (firstTimeCal) {
			makeSettingsTable(str);
			firstTimeCal = false;
		}
		return;
	}
	else {
		str = getItem("getSettings");
		makeSettingsTable(str);
	}
}

function save() {
	getItem("saveSettings");
}

function cancel() {
	getItem("cancelSettings");
}


function setCalFunction(row, coll) {
	console.log("Row index:" + row + " Collumn: " + coll);
	var item = settingsTbl.rows[row].cells[0].innerText;
		//	var x = calTbl.rows[2].cells[3].firstChild.value;
	var value = settingsTbl.rows[row].cells[1].firstChild.value;
	console.log(item + value);
	sendItem("setVal:" + item + '=' + value);
}


function testInfo() {
	var str = "Meting,xActueel,xOffset,xx,\naap,2,3,4,\nnoot,5,6,7,\n,";
	readInfo(str);
	str = "Actueel,Nieuw,Stel in,Herstel,\nSensor 1\n";
	makeNameTable(str);
}

function testCal() {
	var str = "Stroom,Referentie,Stel in,Herstel,\nPositie 1\n Positie 2\n Positie 3\n Positie 4\n";
	readCalInfo(str);
}

function initSettings() {
	if (SIMULATE) {
		testInfo();
		testCal();
	}
	else {
		//document.visibilityState

	}
	readSettingsInfo();

//	setInterval(function() { settingsTimer() }, 1000);
}

function tempCal() {
	var str;
	testCal();
}
var xcntr = 1;

function getInfo() {
	if (SIMULATE) {
		infoTbl.rows[1].cells[1].innerHTML = xcntr++;
		return;
	}
	var arr;
	var str;
	str = getItem("getInfoValues");
	if (firstTime) {
		makeInfoTable(str);
		firstTime = false;
	}
	else {
		var rows = str.split("\n");
		for (var i = 1; i < rows.length - 1; i++) {
			var colls = rows[i].split(",");
			for (var j = 1; j < colls.length; j++) {
				infoTbl.rows[i].cells[j].innerHTML = colls[j];
			}
		}
	}
}

function settingsTimer() {
	if (document.visibilityState == "hidden")
		return;
	getInfo();
}

