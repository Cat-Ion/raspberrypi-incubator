"use strict";
var data, req, tempdata, humdata, tempavg, humavg;

var gettime = function (a) {
	return a[0];
};

var gettemp = function (a) {
	return a[1];
};

var gethum = function (a) {
	return a[2];
};

var drawpart = function (ctx, data, getx, gety) {
	var i = 0;
	ctx.beginPath();
	ctx.moveTo(getx(data[i]), gety(data[i]));
	i++;
	while(i < data.length) {
		ctx.lineTo(getx(data[i]), gety(data[i]));
		i++;
	}
	ctx.stroke();
	ctx.closePath();
};
var findtic = function (dst, min, max, px) {
	var base = Math.pow(10, Math.floor(Math.log(dst/6)/Math.LN10)),
	mintic = base * 1,
	mincoord = px*base/dst,
	coord = 0;
	
	coord = px * (2*base) / dst;
	if((coord < max) && (coord > min)) { mintic = 2 * base; mincoord = coord; }

	coord = px * (4*base) / dst;
	if((coord < max) && (coord > min)) { mintic = 4 * base; mincoord = coord; }

	coord = px * (5*base) / dst;
	if((coord < max) && (coord > min)) { mintic = 5 * base; mincoord = coord; }

	coord = px * (8*base) / dst;
	if((coord < max) && (coord > min)) { mintic = 8 * base; mincoord = coord; }

	return mintic;
};
var drawdata = function () {
	var id = 'graph',
	width = document.getElementById(id).scrollWidth,
	height = document.getElementById(id).scrollHeight,
	ctx = document.getElementById(id).getContext("2d"),
	vertmargin = 5,
	midheight = 20,
	boxheight = (height-midheight) / 2 - vertmargin,
	leftrightmargin = ctx.measureText("100.00   ").width + 8,
	boxwidth = width - leftrightmargin * 2,
	mintemp = Math.round((tempavg[0] - 3 * tempavg[1]) * 10) / 10,
	maxtemp = Math.round((tempavg[0] + 3 * tempavg[1] + 0.05) * 10) / 10,
	tictemp = findtic(maxtemp-mintemp, 20, 60, boxheight),
	minhum = Math.round((humavg[0] - 3 * humavg[1]) * 10) / 10,
	maxhum = Math.round((humavg[0] + 3 * humavg[1] + 0.05) * 10) / 10,
	tichum = findtic(maxhum-minhum, 20, 60, boxheight),
	bottom = boxheight + vertmargin, left = leftrightmargin,
	minx = 0,
	maxx = 3600,
	miny = mintemp,
	maxy = maxtemp,
	i, text, size, x1, x2, y;

	ctx.font = "10px Arial";
	ctx.clearRect(0, 0, width, height);
	ctx.strokeStyle = "#000000";
	ctx.lineWidth = 1;

	ctx.fillText("T",  2, vertmargin + boxheight/2 + 4);
	ctx.fillText("RH", 2, vertmargin + 3*boxheight/2 + 4 + midheight);

	ctx.strokeRect(leftrightmargin, vertmargin, boxwidth, boxheight);
	ctx.strokeRect(leftrightmargin, vertmargin+boxheight+midheight, boxwidth, boxheight);

	for(i = 0; i <= 60; i += 10) {
		size = ctx.measureText(i);
		ctx.fillText(i-60, leftrightmargin + i*boxwidth/60 - size.width/2, vertmargin + boxheight + 14);
	}

	for(i = mintemp; i <= maxtemp; i = Math.round((i + tictemp) * 100)/100) {
		text = i.toPrecision(3);
		size = ctx.measureText(text).width;
		x1 = leftrightmargin - 2 - size;
		x2 = leftrightmargin + boxwidth + 2;
		y  = vertmargin + boxheight*(1-(i-mintemp)/(maxtemp-mintemp)) + 4;
		ctx.fillText(text, x1, y);
		ctx.fillText(text, x2, y);
	}

	for(i = minhum; i <= maxhum; i = Math.round((i + tichum) * 100)/100) {
		text = i.toPrecision(3);
		size = ctx.measureText(text).width;
		x1 = leftrightmargin - 2 - size;
		x2 = leftrightmargin + boxwidth + 2;
		y  = vertmargin + boxheight + midheight + boxheight*(1-(i-minhum)/(maxhum-minhum)) + 4;
		ctx.fillText(text, x1, y);
		ctx.fillText(text, x2, y);
	}

	ctx.save();
	ctx.strokeStyle="#FF0000";

	ctx.setTransform(boxwidth/(maxx - minx),
					 0,
					 0,
					 -boxheight/(maxy - miny),
					 left    - boxwidth  * minx/(maxx-minx),
					 bottom  + boxheight * miny/(maxy-miny));
	ctx.lineWidth = (maxy-miny)/boxheight;
	drawpart(ctx, data, gettime, gettemp);

	bottom = height - vertmargin;
	miny = minhum;
	maxy = maxhum;
	ctx.setTransform(boxwidth/(maxx - minx),
					 0,
					 0,
					 -boxheight/(maxy - miny),
					 left    - boxwidth  * minx/(maxx-minx),
					 bottom  + boxheight * miny/(maxy-miny));
	ctx.lineWidth = (maxy-miny)/boxheight;
	drawpart(ctx, data, gettime, gethum);

	ctx.restore();
};
var replaceChild = function (id, text) {
	var mod = document.getElementById(id);
	mod.removeChild(mod.firstChild);
	mod.appendChild(document.createTextNode(text));
};
var update = function () {
	var now = Date.now();
	data = JSON.parse(req.responseText);
	tempavg = data.temp;
	humavg = data.hum;

	replaceChild('tempavg', data.temp[0] + "\u00B1" + data.temp[1]);
	replaceChild('tempsoll', data.temp[2]);
	replaceChild('humavg', data.hum[0] + "\u00B1" + data.hum[1]);
	replaceChild('humsoll', data.hum[2]);
	data = data.logdata.map(function(d) {
		return [ 3600 + (Date.parse(d[0]) - now)/1000,
				 d[1],
				 d[2]
			   ]; });
	drawdata();
};
var requestupdate = function () {
	req = new XMLHttpRequest();
	req.overrideMimeType("application/json");
	req.onload = update;
	req.open("GET", "/log.json");
	req.send();
};
var resizeCanvas = function() {
	var g = document.getElementById('graph');
	g.setAttribute('width', window.innerWidth-20);
	g.setAttribute('height', window.innerHeight*0.5);
}

window.onresize = function() {
	resizeCanvas();
	update();
};

var cd = document.getElementById('canvasdiv'),
canvas = document.createElement('canvas');
canvas.setAttribute('id', 'graph');
cd.appendChild(canvas);
resizeCanvas();
requestupdate();
window.setInterval(requestupdate, 2000);
