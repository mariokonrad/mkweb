var make_gpx_layer = function(map, gpx_url) {
	return new OpenLayers.Layer.Vector(
		gpx_url,
		{
			strategies: [new OpenLayers.Strategy.Fixed()],
			protocol: new OpenLayers.Protocol.HTTP({
				url: gpx_url,
				format: new OpenLayers.Format.GPX()
			}),
			style: {
				strokeColor: "blue",
				strokeWidth: 5,
				strokeOpacity: 0.5
			},
			projection: map.displayProjection
		});
}

var make_poi_layer = function(map, poi_url) {
	return new OpenLayers.Layer.Text(
		poi_url,
		{
			location: poi_url,
			projection: map.displayProjection
		});
}

var create_map = function(div_id, cfg) {
	var map = new OpenLayers.Map(div_id, {
		div: div_id,
		projection: new OpenLayers.Projection("EPSG:900913"),
		displayProjection: new OpenLayers.Projection("EPSG:4326"),
		numZoomLevels: 19,
		units: 'm',
		controls: [
			new OpenLayers.Control.Navigation(),
			new OpenLayers.Control.PanZoom(),
			new OpenLayers.Control.MousePosition(),
		],
	});

	map.addLayer(new OpenLayers.Layer.OSM("OSM Map"));

	if ('gpx' in cfg) {
		map.addLayer(make_gpx_layer(map, cfg['gpx']));
	}
	if ('poi' in cfg) {
		map.addLayer(make_poi_layer(map, cfg['poi']));
	}

	var lon = 0.0;
	var lat = 0.0;
	var zoom = 1;

	if ('lon' in cfg) {
		lon = 0.0 + cfg['lon'];
	}
	if ('lat' in cfg) {
		lat = 0.0 + cfg['lat'];
	}
	if ('zoom' in cfg) {
		zoom = 0 + cfg['zoom'];
	}

	map.setCenter(
		new OpenLayers.LonLat(lon, lat).transform(
			new OpenLayers.Projection("EPSG:4326"),
			map.getProjectionObject()),
		zoom);

	return map;
}
