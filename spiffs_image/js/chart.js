var CO2data;
var tdata;
var RHdata;

var chartRdy = false;
var tick = 0;
var dontDraw = false;
var halt = false;
var chartHeigth = 500;
var simValue1 = 0;
var simValue2 = 0;
var table;
var presc = 1;
var simMssgCnts = 0;
var lastTimeStamp = 0;
var REQINTERVAL = (5 * 60); // sec
var firstRequest = true;
var plotTimer = 6; // every 60 seconds plot averaged value
var rows = 0;
var chargeInfoTbl;

var SECONDSPERTICK = (1 * 60);// log interval 
var LOGDAYS = 1;
var MAXPOINTS = (LOGDAYS * 24 * 60 * 60 / SECONDSPERTICK)

var dayNames = ['zo', 'ma', 'di', 'wo', 'do', 'vr', 'za'];
var displayNames = ["Ref", "S1", "S2", "S3", "S4"];
var cbIDs = ["S0cb", "S1cb", "S2cb", "S3cb", "S4cb"];

var chartSeries = [-1, -1, -1, -1, -1];
var sensorStatus = [-1, -1, -1, -1, -1];

var NRSensors = displayNames.length;
var NRFields = 4; // timestamp, co2, t , rh 

var CO2Options = {
	title: '',
	curveType: 'function',
	legend: { position: 'top' },
	heigth: 200,
	crosshair: { trigger: 'both' },	// Display crosshairs on focus and selection.
	explorer: {
		actions: ['dragToZoom', 'rightClickToReset'],
		//actions: ['dragToPan', 'rightClickToReset'],
		axis: 'horizontal',
		keepInBounds: true,
		maxZoomIn: 100.0
	},
	chartArea: { 'width': '90%', 'height': '80%' },
};

function clear() {
	CO2data.removeRows(0, CO2data.getNumberOfRows());
	tdata.removeRows(0, tdata.getNumberOfRows());
	RHdata.removeRows(0, RHdata.getNumberOfRows());
	CO2chart.draw(CO2data, CO2Options);
	RHchart.draw(RHdata, CO2Options);
	tchart.draw(tdata, CO2Options);
}

//var formatter_time= new google.visualization.DateFormat({formatType: 'long'});
// channel 1 .. 5

function initChart() {
	window.addEventListener('load', loadCBs());
}

function loadCBs() {
	var cbstate;
	console.log("Reading CBs");
	// Get the current state from localstorage
	// State is stored as a JSON string
	cbstate = JSON.parse(localStorage['CBState'] || '{}');
	// Loop through state array and restore checked 
	// state for matching elements
	for (var i in cbstate) {
		var el = document.querySelector('input[name="' + i + '"]');
		if (el) el.checked = true;
	}
	// Get all checkboxes that you want to monitor state for
	var cb = document.getElementsByClassName('save-cb-state');
	// Loop through results and ...
	for (var i = 0; i < cb.length; i++) {
		//bind click event handler
		cb[i].addEventListener('click', function (evt) {
			// If checkboxe is checked then save to state
			if (this.checked) {
				cbstate[this.name] = true;
			}
			// Else remove from state
			else if (cbstate[this.name]) {
				delete cbstate[this.name];
			}
			// Persist state
			localStorage.CBState = JSON.stringify(cbstate);
		});
	}
	console.log("CBs read");
	initChart2();
};

function getSensorStatus () {
	var str = getItem( 'getSensorStatus'); // S0, x, y S1.. status , mssg cntr
	var arr = str.split("S");
	for ( var m = 1 ; m < arr.length; m++) {
		var arr2 = arr[m].split(","); 
		var sensornr = arr2[0];
		sensorStatus[sensornr] = arr2[1];
	}
}

function initChart2() {

	var activeSeries = 0;
	CO2chart = new google.visualization.LineChart(document.getElementById('CO2chart'));
	CO2data = new google.visualization.DataTable();
	CO2data.addColumn('string', 'Time');

	tchart = new google.visualization.LineChart(document.getElementById('tchart'));
	tdata = new google.visualization.DataTable();
	tdata.addColumn('string', 'Time');

	RHchart = new google.visualization.LineChart(document.getElementById('RHchart'));
	RHdata = new google.visualization.DataTable();
	RHdata.addColumn('string', 'Time');

	getSensorStatus();

	for (var m = 0; m < NRSensors; m++) {
		var cb = document.getElementById(cbIDs[m]);
		if (cb) {
			if (cb.checked) {
				CO2data.addColumn('number', displayNames[m] + ":CO2");
				tdata.addColumn('number', displayNames[m] + ':t');
				RHdata.addColumn('number', displayNames[m] + ':RV');
				chartSeries[m] = activeSeries;
				activeSeries++;
			}
		}
	}
	if (activeSeries == 0) {  // none cb in
		CO2data.addColumn('number', displayNames[1] + ":CO2");
		tdata.addColumn('number', displayNames[1] + ':t');
		RHdata.addColumn('number', displayNames[1] + ':RV');
		chartSeries[1] = 1;
		cb = document.getElementById(cbIDs[1]);
		cb.checked = true;
		}
	if (SIMULATE) {
		simplot();
	//	plotTest();
	}

	//SIMULATE = true;	
	startTimer();
}


function startTimer() {
	setInterval(function () { timer() }, 1000);
}

function plotTest() {
	var p = 10;
	for (var n = 0; n < 3; n++) {
		plot( 1,  50, p+20, p+40, n);
		plot( 1, 50, p+20, p+40, n );
		plot( 1, 50, p+20, p+40,n) ;
		p+= 5;
		plot( 2,  50, p+20, p+40, n);
		plot( 2, 50, p+20, p+40, n );
		plot( 2, 50, p+20, p+40,n) ;
	}	

	// for (var n = 0; n < 3; n++) {
	// 	RHdata.addRow();
	// 	tdata.addRow();
	// 	CO2data.addRow();
	// 	var row = CO2data.getNumberOfRows() - 1;
	// 	// if (CO2data.getNumberOfRows() > MAXPOINTS == true) {
	// 	// 	CO2data.removeRows(0, CO2data.getNumberOfRows() - MAXPOINTS);
	// 	// 	tempAndRHdata.removeRows(0, CO2data.getNumberOfRows() - MAXPOINTS);
	// 	// }
	// 	// //	var date = new Date(timeStamp);
	// 	//	var labelText = date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
	// 	var labelText = n + 1 .toString();
	// 	CO2data.setValue(row, 0, labelText);
	// 	RHdata.setValue(row, 1, 0, labelText);
	// 	tdata.setValue(row, 1, 0, labelText);

	// 	var value = n + 1
	// 	CO2data.setValue(row, 1, value);
	// 	CO2data.setValue(row, 2, value + 0.2);
	// 	value = n + 1;
	// 	RHdata.setValue(row, 1, value + 0.1);
	// 	tempAndRHdata.setValue(row, 2, value + 30);
	// 	value = n + 2;
	// 	tempAndRHdata.setValue(row, 3, value + 0.1);
	// 	//	tempAndRHdata.setValue(row, 4, value +35);
	// }
	RHchart.draw(RHdata, CO2Options);
	tchart.draw(tdata, CO2Options);
	CO2chart.draw(CO2data, CO2Options);
}

function plot(channel, co2, t, rh, timeStamp) {
	var row;
	if (channel == 0) {
		RHdata.addRow();
		tdata.addRow();
		CO2data.addRow();
		row = CO2data.getNumberOfRows();
		if (row > MAXPOINTS ) {
			CO2data.removeRows(0, 1);
			RHdata.removeRows(0, 1);
			tdata.removeRows(0, 1);
			row--;
		}
		var date = new Date(timeStamp);
		var labelText = date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();

		CO2data.setValue(row - 1 , 0, labelText);
		RHdata.setValue(row - 1, 0, labelText);
		tdata.setValue(row - 1, 0, labelText);
	}
	var value = parseFloat(co2);
	row = CO2data.getNumberOfRows() - 1;

	channel= channel+1;
	
	//	console.log("row: " + row + " channel: " + channel + " value: " + value);
	CO2data.setValue(row, channel, value);
	value = parseFloat(t);
	console.log("row: " + row + " channel: " + channel + " value: " + value);
	tdata.setValue(row, channel, value);
	value = parseFloat(rh);
	console.log("row: " + row + " channel: " + channel + " value: " + value);
	RHdata.setValue(row, channel, value);
	console.log(" ");
}

// S1, ts, co21, temp1, hum1, S2, ts, co22, temp2, hum2 S3 ts, co23, temp3, hum3 S4 ts, co24, temp4, hum4\n     
function plotLog(str) {
	var arr;
	var timeOffset;
	var sampleTime;
	var measTimeLastSample;

	var arr2 = str.split("\n");
	var nrPoints = arr2.length - 1;  // blocks of 4 sensors
	if (nrPoints > 0) {
		var arr3 = arr2[nrPoints - 1].split("S");  // 
		arr = arr3[1].split(",");
		measTimeLastSample = arr[1];
		//	document.getElementById('valueDisplay').innerHTML = arr[1] + " " + arr[2]; // value of last measurement
		var sec = Date.now();//  / 1000;  // mseconds since 1-1-1970 
		timeOffset = sec - parseFloat(measTimeLastSample) * 1000;

		for (var p = 0; p < nrPoints; p++) {
			arr3 = arr2[p].split("S"); // S1 x x,y,z S2 .. S4
			for (var m = 0; m < NRSensors; m++) {
				if (chartSeries[m] != -1) {
					arr = arr3[m + 1].split(",");  // sensornr, ts , 
					if (arr.length >= NRFields) {
						sampleTime = parseFloat(arr[1]) * 1000 + timeOffset;
						plot(chartSeries[m], arr[2], arr[3], arr[4], sampleTime);
					}
				}
			}
		}
		CO2chart.draw(CO2data, CO2Options);
		RHchart.draw(RHdata, CO2Options);
		tchart.draw(tdata, CO2Options);
	}
}

function simplot() {
	var str = "S0,1,460,21.1,30  S1,1,460,21.1,30 S2,1,560,25.1,65 S3,1,660,8,70 S4,1,460,21.1,99\n";
	str += "S0,1,460,21.9,32 S1,2,470,21.5,35 S2,1,570,26.1,65 S3,1,670,9,72 S4,1,460,21.1,99\n";
	str += "S0,1,460,29.1,70 S1,3,480,21.7,40 S2,1,590,25.1,60 S3,1,680,10,65 S4,1,460,21.1,110\n";
	plotLog(str);

}

function timer() {
	var arr;
	var str;

	if (SIMULATE) {
		//	simplot();
	}
	else {
		if (firstRequest) {
			arr = getItem("getLogMeasValues");
			CO2data.removeRows(0, CO2data.getNumberOfRows());
			RHdata.removeRows(0, RHdata.getNumberOfRows());
			tdata.removeRows(0, tdata.getNumberOfRows());

			plotLog(arr);
			firstRequest = false;
		}
		str = getItem("getRTMeasValues"); // request data from enabled sensors
		if (str) {
			arr = str.split(",");
			if (arr.length >= 3) {
				if (arr[1] > 0) {
					if (arr[1] != lastTimeStamp) {
						lastTimeStamp = arr[1];
						plotLog(str);
					}
				}
			}
		}
		else
			console.log("getRTMeasValues failed");
	}
}

function clearChart() {
	RHdata.removeRows(0, RHdata.getNumberOfRows());
	tdata.removeRows(0, tdata.getNumberOfRows());
	CO2data.removeRows(0, CO2data.getNumberOfRows());
	RHchart.draw(RHdata, CO2Options);
	tchart.draw(tdata, CO2Options);
	CO2chart.draw(CO2data, CO2Options);
}

function clearLog() {
	sendItem("clearLog");
	clearChart();
}

function refreshChart() {
	RHdata.removeRows(0, RHdata.getNumberOfRows());
	tdata.removeRows(0, tdata.getNumberOfRows());
	CO2data.removeRows(0, CO2data.getNumberOfRows());
	arr = getItem("getLogMeasValues");
	plotArray(arr);
}
