var data;
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

var SECONDSPERTICK = (5 * 60);// log interval 
var LOGDAYS = 1;
var MAXPOINTS = (LOGDAYS * 24 * 60 * 60 / SECONDSPERTICK)

var dayNames = ['zo', 'ma', 'di', 'wo', 'do', 'vr', 'za'];
var displayNames = ["", "S1", "S2", "S3", "S4"];
var cbIDs = ["", "S1cb", "S2cb", "S3cb", "S4cb"];

var chartSeries = [-1, -1, -1, -1, -1];

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

var tempAndRHoptions = {
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

	vAxes: {
		0: { logScale: false },
		1: { logScale: false }
	},
	series: {
		0: { targetAxisIndex: 0 },// temperature
		1: { targetAxisIndex: 1 },// RH
	},
};

function clear() {
	tempAndRHdata.removeRows(0, tempAndRHdata.getNumberOfRows());
	CO2Data.removeRows(0, CO2Data.getNumberOfRows());
	tRHchart.draw(tempAndRHdata, tempAndRHoptions);
	CO2chart.draw(CO2Data, CO2Options);
	tick = 0;
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

function initChart2() {
	CO2chart = new google.visualization.LineChart(document.getElementById('CO2chart'));
	CO2Data = new google.visualization.DataTable();
	CO2Data.addColumn('string', 'Time');
	tRHchart = new google.visualization.LineChart(document.getElementById('tRHchart'));
	tempAndRHdata = new google.visualization.DataTable();
	tempAndRHdata.addColumn('string', 'Time');

	for (var m = 1; m < NRSensors; m++) {
		var cb = document.getElementById(cbIDs[m]);
		if (cb) {
			if (cb.checked) {
				CO2Data.addColumn('number', displayNames[m] + ":CO2");
				tempAndRHdata.addColumn('number', displayNames[m] + ':t');
				tempAndRHdata.addColumn('number', displayNames[m] + ':RH');
				chartSeries[m] = activeSeries;
				activeSeries++;
			}
		}
	}
	//SIMULATE = true;	
	startTimer();
}

function plot(channel, co2, t, rh, timeStamp) {
	tempAndRHdata.addRow();
	CO2data.addRow();
	if (CO2data.getNumberOfRows() > MAXPOINTS == true) {
		CO2data.removeRows(0, CO2data.getNumberOfRows() - MAXPOINTS);
		tempAndRHdata.removeRows(0, CO2data.getNumberOfRows() - MAXPOINTS);
	}
	value = parseFloat(co2);
	CO2data.setValue(CO2data.getNumberOfRows() - 1, channel, value);
	value = parseFloat(t);
	tempAndRHdata.setValue(tempAndRHdata.getNumberOfRows() - 1, channel, value);
	value = parseFloat(rh);
	tempAndRHdata.setValue(tempAndRHdata.getNumberOfRows() - 1, channel * 2, value);
	if (channel == 1) {
		var date = new Date(timeStamp);
		var labelText = date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
		CO2data.setValue(chartData.getNumberOfRows() - 1, 0, labelText);
		tempAndRHdata.setValue(chartData.getNumberOfRows() - 1, 0, labelText);
	}
}

// 
function plotArray(channel, str) {
	var arr;
	var timeOffset;
	var sampleTime;
	var measTimeLastSample;

	var arr2 = str.split("\n");
	var nrPoints = arr2.length - 1;
	if (nrPoints > 0) {
		arr = arr2[nrPoints - 1].split(",");
		measTimeLastSample = arr[0];  // can be unadjusted time in sec units
		//	document.getElementById('valueDisplay').innerHTML = arr[1] + " " + arr[2]; // value of last measurement
		var sec = Date.now();//  / 1000;  // mseconds since 1-1-1970 
		timeOffset = sec - parseFloat(measTimeLastSample) * 1000;
		for (var p = 0; p < nrPoints; p++) {
			arr = arr2[p].split(",");
			if (arr.length >= NRFields) {
				for (var m = 1; m < NRSensors; m++) { // time not used for now
					if (chartSeries[m] != -1) {
						sampleTime = parseFloat(arr[0]) * 1000 + timeOffset;
						plot(chartSeries[m], arr[1], arr[2], arr[3], sampleTime);
					}
				}
			}
		}
		tRHchart.draw(tempAndRHdata, tempAndRHoptions);
		CO2chart.draw(CO2Data, CO2Options);
	}
}

function timer() {
	var arr;
	var arr2;
	var str;

	if (SIMULATE) {
		simplot();
	}
	else {
		// if (firstRequest) {
		// 	arr = getItem("getLogMeasValues");
		// 	tempAndRHdata.removeRows(0, tempAndRHdata.getNumberOfRows());
		// 	CO2Data.removeRows(0, CO2Data.getNumberOfRows());
		// 	plotArray(arr);
		// 	firstRequest = false;
		// }

		for (var m = 1; m < NRSensors; m++) {
			if (chartSeries[m] != -1) {
				str = getItem("getRTMeasValues" + chartSeries[m].toString()); // request data from enabled sensors
				arr = str.split(",");
				if (arr.length >= 3) {
					if (arr[0] > 0) {
						if (arr[0] != lastTimeStamp) {
							lastTimeStamp = arr[0];
							for (var m = 1; m < 4; m++) { // time not used for now 
								var value = parseFloat(arr[m]); // from string to float
//								document.getElementById(displayNames[m]).innerHTML = arr[m] + unit[m];
							}
							var sampleTime = Date.now();
							plot(chartSeries[m], arr[1],arr[2],arr[3], sampleTime);
							tRHchart.draw(tempAndRHdata, tempAndRHoptions);
							CO2chart.draw(CO2Data, CO2Options);
						}
					}
				}
			}
		}
	}
}

function clearChart() {
	tempAndRHdata.removeRows(0, tempAndRHdata.getNumberOfRows());
	CO2Data.removeRows(0, CO2Data.getNumberOfRows());
	tRHchart.draw(tempAndRHdata, tempAndRHoptions);
	CO2chart.draw(CO2Data, CO2Options);
}

function clearLog() {
	sendItem("clearLog");
	clearChart();
}

function refreshChart() {
	data.removeRows(0, data.getNumberOfRows());
	arr = getItem("getLogMeasValues");
	plotArray(arr);
}
