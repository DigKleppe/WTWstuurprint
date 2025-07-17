var CO2data;
var tempAndRHdata;

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
	CO2data.removeRows(0, CO2data.getNumberOfRows());
	tRHchart.draw(tempAndRHdata, tempAndRHoptions);
	CO2chart.draw(CO2data, CO2Options);
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
	var activeSeries = 1;
	CO2chart = new google.visualization.LineChart(document.getElementById('CO2chart'));
	CO2data = new google.visualization.DataTable();
	CO2data.addColumn('string', 'Time');
	tRHchart = new google.visualization.LineChart(document.getElementById('tRHchart'));
	tempAndRHdata = new google.visualization.DataTable();
	tempAndRHdata.addColumn('string', 'Time');

	for (var m = 1; m < NRSensors; m++) {
		var cb = document.getElementById(cbIDs[m]);
		if (cb) {
			if (cb.checked) {
				CO2data.addColumn('number', displayNames[m] + ":CO2");
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


function startTimer() {
	setInterval(function () { timer() }, 1000);
}


function plot(channel, co2, t, rh, timeStamp) {
	if (channel == 1) {
		tempAndRHdata.addRow();
		CO2data.addRow();
		if (CO2data.getNumberOfRows() > MAXPOINTS == true) {
			CO2data.removeRows(0, CO2data.getNumberOfRows() - MAXPOINTS);
			tempAndRHdata.removeRows(0, CO2data.getNumberOfRows() - MAXPOINTS);
		}

		var date = new Date(timeStamp);
		var labelText = date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
		CO2data.setValue(CO2data.getNumberOfRows() - 1, 0, labelText);
		tempAndRHdata.setValue(tempAndRHdata.getNumberOfRows() - 1, 0, labelText);
	}
	value = parseFloat(co2);
	CO2data.setValue(CO2data.getNumberOfRows() - 1, channel, value);
	value = parseFloat(t);
	tempAndRHdata.setValue(tempAndRHdata.getNumberOfRows() - 1, channel, value);
	value = parseFloat(rh);
	tempAndRHdata.setValue(tempAndRHdata.getNumberOfRows() - 1, channel * 2, value);

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
			for (var m = 0; m < 4; m++) {
				if (chartSeries[m + 1] != -1) {
					arr = arr3[m + 1].split(",");  // sensornr, ts , 
					if (arr.length >= NRFields) {
						sampleTime = parseFloat(arr[1]) * 1000 + timeOffset;
						plot(chartSeries[m + 1], arr[2], arr[3], arr[4], sampleTime);
					}
				}
			}
		}
		tRHchart.draw(tempAndRHdata, tempAndRHoptions);
		CO2chart.draw(CO2data, CO2Options);
	}
}

function simplot() {
	var str = "S1,1,460,21.1,65 S2,1,560,21.1,65 S3,1,660,21.1,65 S4,1,460,21.1,65\n";
	str += "S1,2,470,21.5,65 S2,1,570,21.1,65 S3,1,670,21.1,65 S4,1,460,21.1,69\n";
	str += "S1,3,480,21.7,65 S2,1,590,21.1,65 S3,1,680,21.1,65 S4,1,460,21.1,165\n";
	plotLog(str);

}

function timer() {
	var arr;
	var str;

	if (SIMULATE) {
		simplot();
	}
	else {
		if (firstRequest) {
			arr = getItem("getLogMeasValues");
			tempAndRHdata.removeRows(0, tempAndRHdata.getNumberOfRows());
			CO2data.removeRows(0, CO2data.getNumberOfRows());
			plotLog(arr);
			firstRequest = false;
		}
		for (var n = 1; n < NRSensors; n++) {
			if (chartSeries[n] != -1) {
				str = getItem("getRTMeasValues" + chartSeries[n].toString()); // request data from enabled sensors
				if (str) {
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
								plot(chartSeries[n], arr[1], arr[2], arr[3], sampleTime);
								tRHchart.draw(tempAndRHdata, tempAndRHoptions);
								CO2chart.draw(CO2data, CO2Options);
							}
						}
					}
				}
				else
					console.log("getRTMeasValues failed");
			}
		}
	}
}

function clearChart() {
	tempAndRHdata.removeRows(0, tempAndRHdata.getNumberOfRows());
	CO2data.removeRows(0, CO2data.getNumberOfRows());
	tRHchart.draw(tempAndRHdata, tempAndRHoptions);
	CO2chart.draw(CO2data, CO2Options);
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
