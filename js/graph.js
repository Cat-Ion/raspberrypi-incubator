"use strict";
var req, day_req,
    data, day_data,
    templim_h, humlim_h,
    templim_d, humlim_d;

var gettime = function (a) {
	return a[0];
};

var gettemp = function (a) {
	return a[1];
};

var gethum = function (a) {
	return a[2];
};

var gettics = function(min, max, range, density_t) {
	var Q = [1,5,2,2.5,4,3];
	var w = [0.25, 0.25, 0.5];
	var result;
	var best_score = -200;
	
	if(min == max) {
		return { 'min': min - 0.5,
				 'max': min + 0.5,
				 'step': 0.2,
				 'score': 1
			   };
	}

	var q, j, shat, k, dhat, delta, z, lstep, chat, start, s, d, c, lmin, lmax;

	var density = function(r) {
		return 2 - Math.max(density_t/r, r/density_t);
	};
	var density_max = function(r) {
		if(r > density_t) {
			return 1;
		} else {
			return 2 - density_t / r;
		}
	}
	var simplicity = function(q, j, lmin, lmax, lstep) {
		var v = ((Math.abs(lmin)%lstep < 1e-4) && lmin <= 0 && lmax >= 0) ? 1 : 0;
		return 1 + v - q/5 - j;
	};
	var simplicity_max = function(q, j) {
		return 2 - q/5 - j;
	};
	var coverage = function(lmin, lmax) {
		return 1 - 0.5*((max-lmax)*(max-lmax) + (min-lmin)*(min-lmin))/(0.01*(max-min)*(max-min))
	};
	var coverage_max = function(span) {
		var range = max-min;
		if(span > range) {
			var half = (span-range)/2
			return 1 - 0.5 * (half*half+half*half)/(0.01*(max-min)*(max-min))
		} else {
			return 1;
		}
	};

	for(q = 0; q < Q.length; q++) {
		for(j = 1; j <= 20; j++) {
			shat = simplicity_max(q, j);
			if(shat*w[0] + w[1] + w[2] < best_score) {
				break;
			}
			for(k = 2; k < 20; k++) {
				dhat = density_max(k/range);
				if(shat*w[0] + w[1] + dhat*w[2] < best_score) {
					continue;
				}
				delta = (max-min)/(k+1)/(j*Q[q]);
				for(z = Math.floor(Math.log(delta)/Math.LN10); z < 4; z++) {
					//console.log([q,j,k,z]);
					lstep = Q[q] * j * Math.pow(10, z);
					chat = coverage_max((k-1)*lstep);
					if(shat*w[0] + chat * w[1] + dhat * w[2] < best_score) {
						continue;
					}
					var stmin = Math.floor(min/lstep)-k+1,
					stmax = Math.ceil(max/lstep);
					for(start = 0; start <= (stmax-stmin)*j; start++) {
						lmin = stmin*lstep + start*lstep/j;
						lmax = lmin + (k-1)*lstep;
						s = simplicity(q, j, lmin, lmax, lstep);
						d = density(k/range * (lmax-lmin)/(max-min));
						c = coverage(lmin, lmax);

						if(s * w[0] + c * w[1] + d * w[2] < best_score) {
							continue;
						}

						best_score = s * w[0] + c * w[1] + d * w[2];
						result = { 'min': lmin,
								   'max': lmax,
								   'step': lstep,
								   'prec': z - (Q[q] == 2.5 ? 1 : 0),
								   'score': s * w[0] + c * w[1] + d * w[2]
								 };
					}
				}
			}
		}
	}
	return result;
}

var drawgraph = function(ctx,
			  px_x_min, px_x_max,
			  px_y_min, px_y_max,
			  data,
			  data_x_min, data_x_max,
			  data_y_min, data_y_max,
			  getx, gety
			 ) {
	var i;
	var px_x_diff = px_x_max - px_x_min;
	var px_y_diff = px_y_max - px_y_min;
	var data_x_diff = data_x_max - data_x_min;
	var data_y_diff = data_y_max - data_y_min;

	ctx.save();
	ctx.strokeStyle = "#000000";
	ctx.lineWidth = 1;

	ctx.strokeRect(px_x_min, px_y_min, px_x_diff, px_y_diff);

	ctx.strokeStyle = "#FF0000";
	ctx.lineWidth = 1;
	
	i = 0;
	while(getx(data[i]) < 0) {
		i++;
	}

	ctx.beginPath();
	ctx.moveTo(
			px_x_min + (getx(data[i]) - data_x_min) * px_x_diff/data_x_diff,
			px_y_max - (gety(data[i]) - data_y_min) * px_y_diff/data_y_diff
	);
	i++;
	while(i < data.length) {
		ctx.lineTo(
			px_x_min + (getx(data[i]) - data_x_min) * px_x_diff/data_x_diff,
			px_y_max - (gety(data[i]) - data_y_min) * px_y_diff/data_y_diff
		);
		i++;
	}
	ctx.stroke();
	ctx.closePath();
	
	ctx.restore();
}

var drawtics = function(ctx,
						ticmin, ticmax, ticstep, ticprecision,
						getx, gety
					   ) {
	var i, text, size;
	var prec = ticprecision <= 0 ? -ticprecision : 0;
	for(i = 0; ticmin + i*ticstep <= ticmax; i++) {
		text = (ticmin + i*ticstep).toFixed(prec);
		size = ctx.measureText(text).width;
		ctx.fillText(text, getx(ticmin + i * ticstep, size), gety(ticmin + i * ticstep, size));
	}
}

var drawTimeTics = function(ctx,
							ticmin, ticmax, ticstep,
							getx, gety) {
	var i, text, size;
	for(i = 0; ticmin + i*ticstep <= ticmax; i++) {
		var d = new Date(Date.now() + 1000 * (ticmin + i*ticstep));
		text = d.getHours() + ":" + (d.getMinutes() < 10 ? "0" : "") + d.getMinutes();
		size = ctx.measureText(text).width;
		ctx.fillText(text, getx(ticmin + i * ticstep, size), gety(ticmin + i * ticstep, size));
	}
	
}

var graph = function(id,
		  time,
		  data,
		  templim, humlim
		 ) {
	var ctx = document.getElementById(id).getContext("2d");
	var width = document.getElementById(id).scrollWidth;
	var height = document.getElementById(id).scrollHeight;
	var vertpadding = 5;
	var midpadding = 20;
	var rightpadding = ctx.measureText("100.000").width + 5;
	var leftpadding = rightpadding + 20;

	var boxwidth = width - rightpadding - leftpadding;
	var boxheight = (height - 2*vertpadding - midpadding) / 2;

	ctx.clearRect(0, 0, width, height);

	var ymin = Math.floor(templim[0]*5-1)/5;
	var ymax = Math.ceil(templim[1]*5+1)/5;
	var tics = gettics(ymin, ymax, boxheight, 1/20);
	if(tics.min < ymin) { ymin = tics.min; }
	if(tics.max > ymax) { ymax = tics.max; }
	drawtics(ctx, tics.min, tics.max, tics.step, tics.prec,
			 function(v, width) { return leftpadding - 2 - width; },
			 function(v, width) { return vertpadding + boxheight * (1 - (v - ymin)/(ymax - ymin)) + 4 }
			);
	drawgraph(ctx, leftpadding, width - rightpadding,
			  vertpadding, vertpadding + boxheight,
			  data,
			  0, time,
			  ymin, ymax,
			  gettime, gettemp);
	
	ymin = Math.floor(humlim[0]*5-1)/5;
	ymax = Math.ceil(humlim[1]*5+1)/5;
	tics = gettics(ymin, ymax, boxheight, 1/20);
	if(tics.min < ymin) { ymin = tics.min; }
	if(tics.max > ymax) { ymax = tics.max; }
	drawtics(ctx, tics.min, tics.max, tics.step, tics.prec,
			 function(v, width) { return leftpadding - 2 - width; },
			 function(v, width) { return height - vertpadding - boxheight * (v - ymin)/(ymax - ymin) + 4 }
			);

	drawgraph(ctx, leftpadding, width - rightpadding,
			  vertpadding + boxheight + midpadding, height - vertpadding,
			  data,
			  0, time,
			  Math.floor(humlim[0]*2-1)/2, Math.ceil(humlim[1]*2+1)/2,
			  gettime, gethum);

	var timescale = time <= 60 ? 1 : time <= 3600 ? 300 : 3600;
	var timestart = Math.ceil( (Date.now()/1000 - time) / timescale )*timescale - Date.now() / 1000;
	var timeend   = Math.floor( Date.now()/1000/timescale)*timescale - Date.now()/1000; 
	var timetic   = timescale;

	drawTimeTics(ctx, timestart, timeend, timetic,
			 function(v, twidth) { return width - rightpadding + v/time*boxwidth - twidth / 2; },
			 function(v, twidth) { return vertpadding + boxheight + midpadding / 2 + 4; }
			);
}

var replaceChild = function (id, text) {
	var mod = document.getElementById(id);
	mod.removeChild(mod.firstChild);
	mod.appendChild(document.createTextNode(text));
};
var update = function () {
	var now = Date.now();
	templim_h = [100, 0];
	humlim_h = [100, 0];

	data = JSON.parse(req.responseText);

	replaceChild('tempavg', data.temp[0] + "\u00B1" + data.temp[1]);
	replaceChild('tempsoll', data.temp[2]);
	replaceChild('humavg', data.hum[0] + "\u00B1" + data.hum[1]);
	replaceChild('humsoll', data.hum[2]);
	data = data.logdata.map(function(d) {
		if(d[1] < templim_h[0]) { templim_h[0] = d[1]; }
		if(d[1] > templim_h[1]) { templim_h[1] = d[1]; }
		if(d[2] < humlim_h[0]) { humlim_h[0] = d[2]; }
		if(d[2] > humlim_h[1]) { humlim_h[1] = d[2]; }
		return [ 3600 + (Date.parse(d[0]) - now)/1000,
				 d[1],
				 d[2]
			   ]; });
	graph('graph', 3600, data, templim_h, humlim_h);
};
var day_update = function () {
	var now = Date.now();
	templim_d = [100, 0];
	humlim_d = [100, 0];

	day_data = JSON.parse(day_req.responseText);
	day_data = day_data.logdata.map(function(d) {
		if(d[1] < templim_d[0]) { templim_d[0] = d[1]; }
		if(d[1] > templim_d[1]) { templim_d[1] = d[1]; }
		if(d[2] < humlim_d[0]) { humlim_d[0] = d[2]; }
		if(d[2] > humlim_d[1]) { humlim_d[1] = d[2]; }
		return [ 24*3600 + (Date.parse(d[0]) - now)/1000,
				 d[1],
				 d[2]
			   ]; });
	graph('day_graph', 24*3600, day_data, templim_d, humlim_d);
};
var requestupdate = function () {
	req = new XMLHttpRequest();
	req.overrideMimeType("application/json");
	req.onload = update;
	req.open("GET", "/log.json");
	req.send();

	day_req = new XMLHttpRequest();
	day_req.overrideMimeType("application/json");
	day_req.onload = day_update;
	day_req.open("GET", "/daylog.json");
	day_req.send();
};
var resizeCanvas = function(id) {
	var g = document.getElementById(id);
	g.setAttribute('width', window.innerWidth-20);
	g.setAttribute('height', window.innerHeight*0.4);
}

window.onresize = function() {
	resizeCanvas('graph');
	resizeCanvas('day_graph');
	update();
	day_update();
};

var cd = document.getElementById('canvasdiv'),

canvas = document.createElement('canvas');
canvas.setAttribute('id', 'graph');
cd.appendChild(canvas);
resizeCanvas('graph');

canvas = document.createElement('canvas');
canvas.setAttribute('id', 'day_graph');
cd.appendChild(canvas);
resizeCanvas('day_graph');

requestupdate();
window.setInterval(requestupdate, 2000);
gettics(24, 26.2, 532, 1/20);
