function Terrain() {}

Terrain.prototype.place = function(x, z)
{
	g_Map.terrainObjects[x][z] = undefined;
	this.placeNew(x, z);
};

Terrain.prototype.placeNew = function() {};

/**
 * Class for painting the given terrain texture.
 * Optionally places an entity of the given template on each affected tile.
 */
function SimpleTerrain(texture, treeType = undefined)
{
	if (texture === undefined)
		throw new Error("SimpleTerrain: texture not defined");

	this.texture = texture;
	this.treeType = treeType;
}

SimpleTerrain.prototype = new Terrain();
SimpleTerrain.prototype.constructor = SimpleTerrain;
SimpleTerrain.prototype.placeNew = function(x, z)
{
	if (this.treeType !== undefined && g_Map.validT(x, z))
		g_Map.terrainObjects[x][z] = new Entity(this.treeType, 0, x + 0.5, z + 0.5, randFloat(0, 2 * PI));

	g_Map.texture[x][z] = g_Map.getTextureID(this.texture);
};


/**
 * Places one of the given terrains.
 */
function RandomTerrain(terrains)
{
	if (!(terrains instanceof Array) || !terrains.length)
		throw new Error("RandomTerrain: Invalid terrains array");

	this.terrains = terrains;
}

RandomTerrain.prototype = new Terrain();
RandomTerrain.prototype.constructor = RandomTerrain;
RandomTerrain.prototype.placeNew = function(x, z)
{
	pickRandom(this.terrains).placeNew(x, z);
};

/**
 * Global helper functions.
 */
function createTerrain(terrain)
{
	if (!(terrain instanceof Array))
		return createSimpleTerrain(terrain);

	return new RandomTerrain(terrain.map(t => createTerrain(t)));
}

function createSimpleTerrain(terrain)
{
	if (typeof terrain != "string")
		throw new Error("createSimpleTerrain expects string as input, received " + uneval(terrain));

	// Split string by pipe | character, this allows specifying terrain + tree type in single string
	let params = terrain.split(TERRAIN_SEPARATOR, 2);

	if (params.length != 2)
		return new SimpleTerrain(terrain);

	return new SimpleTerrain(params[0], params[1]);
}
