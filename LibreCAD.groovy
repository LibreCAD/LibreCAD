def LibreCadExePath()
{
	return "%WORKSPACE%\\LibreCAD.exe"
}
def Build()
{
	writefile(LibreCadExePath(),"")
}
def GetBuiltFiles()
{
	return LibreCadExePath()
}
def BuildInstaller()
{
	writefile(GetInstallerPath(), "")
}
def GetInstallerPath()
{
	return "%WORKSPACE%\\generated\\LibreCAD-Installer.exe"
}

return this