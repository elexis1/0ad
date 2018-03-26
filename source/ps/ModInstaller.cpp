/* Copyright (C) 2018 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "ModInstaller.h"

#include "lib/file/vfs/vfs_util.h"
#include "ps/Filesystem.h"
#include "ps/XML/Xeromyces.h"

#include <fstream>

CModInstaller::CModInstaller(const OsPath& modsdir, const OsPath& tempdir) :
	m_ModsDir(modsdir), m_TempDir(tempdir / "_modscache"), m_CacheDir("cache/")
{
	m_VFS = CreateVfs();
	CreateDirectories(m_TempDir, 0700);
}

CModInstaller::~CModInstaller()
{
	m_VFS.reset();
	DeleteDirectory(m_TempDir);
}

CStr CModInstaller::Install(const OsPath& mod,
                            const std::shared_ptr<ScriptRuntime>& scriptRuntime,
                            bool deleteAfterInstall)
{
	const OsPath modTemp = m_TempDir / mod.Basename() / mod.Filename().ChangeExtension(L".zip");
	CreateDirectories(modTemp.Parent(), 0700);

	CopyFile(mod, modTemp, true);

	// Load the mod to VFS
	if (m_VFS->Mount(m_CacheDir, m_TempDir / "") != INFO::OK)
		return "";
	CVFSFile modinfo;
	PSRETURN modinfo_status = modinfo.Load(m_VFS, m_CacheDir / modTemp.Basename() / "mod.json", false);
	m_VFS->Clear();
	if (modinfo_status != PSRETURN_OK)
		return "";

	// Extract the name of the mod
	ScriptInterface scriptInterface("Engine", "ModInstaller", scriptRuntime);
	JSContext* cx = scriptInterface.GetContext();
	JS::RootedValue json_val(cx);
	if (!scriptInterface.ParseJSON(modinfo.GetAsString(), &json_val))
		return "";
	JS::RootedObject json_obj(cx, json_val.toObjectOrNull());
	JS::RootedValue name_val(cx);
	if (!JS_GetProperty(cx, json_obj, "name", &name_val))
		return "";
	CStr modName;
	ScriptInterface::FromJSVal(cx, name_val, modName);
	if (modName.empty())
		return "";

	const OsPath modDir = m_ModsDir / modName;
	const OsPath modPath = modDir / (modName + ".zip");

	// Create a directory with the next structure:
	//   mod-name/
	//     mod-name.zip
	CreateDirectories(modDir, 0700);
	wrename(modTemp, modPath);
	DeleteDirectory(modTemp.Parent());

#ifdef OS_WIN
	// On Windows, write the contents of mod.json to a separate file next to the archive:
	//   mod-name/
	//     mod-name.zip
	//     mod.json
	std::ofstream mod_json((modDir / "mod.json").string8());
	if (mod_json.good())
	{
		mod_json << modinfo.GetAsString();
		mod_json.close();
	}
#endif // OS_WIN

	if (deleteAfterInstall)
	{
		// Remove the original file
		wunlink(mod);
	}
	return modName;
}

// static
bool CModInstaller::IsDefaultModExtension(const Path& ext)
{
	return ext == ".pyromod" || ext == ".zip";
}
