var create_map = function(div_id, gpx_url, poi_url) {

	var map = new OpenLayers.Map('map', {
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

	map.addLayer(new OpenLayers.Layer.Vector(
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
			projection: new OpenLayers.Projection("EPSG:4326")
		}));

	map.addLayer(new OpenLayers.Layer.Text(
		poi_url,
		{
			location: poi_url,
			projection: map.displayProjection
		}));

	map.setCenter(
		new OpenLayers.LonLat(-2.5, 42.0).transform(new OpenLayers.Projection("EPSG:4326"),
		map.getProjectionObject()), 5);

	return map;
}
