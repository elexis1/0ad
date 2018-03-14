let g_ModsAvailableOnline = [];

function init()
{
	g_ModsAvailableOnline = Engine.ModIoGetMods();

	generateModsList(g_ModsAvailableOnline);
}

function filesizeToString(filesize)
{
	let suffixes = ["B", "kiB", "MiB", "GiB"]; // bigger values are currently unlikely to occur here...
	let i = 0;
	while (i < suffixes.length-1)
	{
		if (filesize < 1024)
			break;
		filesize /= 1024;
		++i;
	}

	return filesize.toFixed(i == 0 ? 0 : 1) + suffixes[i];
}

function generateModsList(mods)
{
	var [keys, names, name_ids, versions, filesizes, dependencies] = [[],[],[],[],[],[]];

	let i = 0;
	for (let mod of mods)
	{
		keys.push(i++);
		names.push(mod.name);
		name_ids.push(mod.name_id);
		versions.push(mod.version);
		filesizes.push(filesizeToString(mod.filesize));
		dependencies.push((mod.dependencies || []).join(" "));
	}

	var obj = Engine.GetGUIObjectByName("modsAvailableList");
	obj.list_name = names;
	obj.list_modVersion = versions;
	obj.list_modname_id = name_ids;
	obj.list_modfilesize = filesizes;
	obj.list_dependencies = dependencies;

	obj.list = keys;
}

function showModDescription()
{
	let listObject = Engine.GetGUIObjectByName("modsAvailableList");
	if (listObject.selected != -1)
		Engine.GetGUIObjectByName("modDescription").caption = g_ModsAvailableOnline[listObject.selected].summary;
}

function downloadMod()
{
	let listObject = Engine.GetGUIObjectByName("modsAvailableList");
	if (listObject.selected == -1)
	{
		warn("Select something first.");
		return;
	}

	Engine.ModIoDownloadMod(+listObject.list[listObject.selected]);
}

function closePage()
{
	Engine.PopGuiPage();
}
