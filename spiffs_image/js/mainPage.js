var CO2RPMdata;
var RPMdata;

var chartRdy = false;
var tick = 0;

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

var SECONDSPERTICK = (5 * 60);// log interval 
var LOGDAYS = 1;
var MAXPOINTS = (LOGDAYS * 24 * 60 * 60 / SECONDSPERTICK)


var sensorStatus = [-1, -1, -1, -1, -1];


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

	vAxes: {
		0: { logScale: false },
		1: { logScale: false },
	},
	series: {
		0: { targetAxisIndex: 0 },// CO2
		1: { targetAxisIndex: 1 },// RPM
	},

};

function clear() {
	CO2RPMdata.removeRows(0, CO2RPMdata.getNumberOfRows());
	CO2RPMchart.draw(CO2RPMdata, CO2Options);
}

//var formatter_time= new google.visualization.DateFormat({formatType: 'long'});
// channel 1 .. 5

function initChart() {
	CO2RPMchart = new google.visualization.LineChart(document.getElementById('CO2RPMchart'));
	CO2RPMdata = new google.visualization.DataTable();
	CO2RPMdata.addColumn('string', 'Time');
	CO2RPMdata.addColumn('number', "CO2");
	CO2RPMdata.addColumn('number', "Toerental");



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
	CO2RPMchart.draw(CO2RPMdata, CO2Options);
}

function plotCO2RPMLog(co2, rpm, timeStamp) {
	var row;
	CO2RPMdata.addRow();
	row = CO2RPMdata.getNumberOfRows();
	if (row > MAXPOINTS ) {
		CO2RPMdata.removeRows(0, 1);
		row--;
	}
	var date = new Date(timeStamp);
	var labelText = date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
	row--;
	CO2RPMdata.setValue(row , 0, labelText);

	var value = parseFloat(co2);
	CO2RPMdata.setValue(row, 1, value);
	value = parseInt(rpm);
	CO2RPMdata.setValue(row, 2, value);
}

// "S0", ts, co20, temp0, hum0, "S1", ts, co21, temp1, hum1, "S2", ts, co22, temp2, hum2 ,"S3", ts, co23, temp3, hum3, S4 ts, co24, temp4, hum4, co2, RPM\n     
function plotCO2RPMLog(str) {
	var arr;
	var timeOffset;
	var sampleTime;
	var measTimeLastSample;
	var arr2 = str.split("\n");

	var nrPoints = arr2.length - 1;  // blocks of 4 sensors
	if (nrPoints > 0) {
		var arr = arr2[nrPoints - 1].split(",");   
		measTimeLastSample = arr[1];
			
		var sec = Date.now();//  / 1000;  // mseconds since 1-1-1970 
		timeOffset = sec - parseFloat(measTimeLastSample) * 1000;

		for (var p = 0; p < nrPoints; p++) {
			arr = arr2[p].split(",");   
			if (arr.length >= NRFields) {
				sampleTime = parseFloat(arr[1]) * 1000 + timeOffset;
				plot( arr[21], arr[22], sampleTime);
			}
		}
		CO2RPMchart.draw(CO2RPMdata, CO2Options);
	}
}

function simplot() {
	var str = "S0,1,460,21.1,30  S1,1,460,21.1,30 S2,1,560,25.1,65 S3,1,660,8,70 S4,1,460,21.1,99, 450, 2000\n";
	str += "S0,1,460,21.9,32 S1,2,470,21.5,35 S2,1,570,26.1,65 S3,1,670,9,72 S4,1,460,21.1,99, 500, 2200\n";
	str += "S0,1,460,29.1,70 S1,3,480,21.7,40 S2,1,590,25.1,60 S3,1,680,10,65 S4,1,460,21.1,110, 600, 3000\n";
	plotCO2RPMLog(str);

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
			CO2RPMdata.removeRows(0, CO2RPMdata.getNumberOfRows());
			RHdata.removeRows(0, RHdata.getNumberOfRows());
			RPMdata.removeRows(0, RPMdata.getNumberOfRows());

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
	CO2RPMdata.removeRows(0, CO2RPMdata.getNumberOfRows());
	CO2chart.draw(CO2RPMdata, CO2Options);
}

function clearLog() {
	getItem("clearLog");
	clearChart();
}

function refreshChart() {
	CO2RPMdata.removeRows(0, CO2RPMdata.getNumberOfRows());
	arr = getItem("getLogMeasValues");
	plotArray(arr);
}
