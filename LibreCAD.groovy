def LibreCadExePath()
{
	return "%WORKSPACE%\\LibreCAD.exe"
}
def Build()
{
	writeFile(LibreCadExePath(),"")
}
def GetBuiltFiles()
{
	return LibreCadExePath()
}
def BuildInstaller()
{
	writeFile(GetInstallerPath(), "")
}
def GetInstallerPath()
{
	return "%WORKSPACE%\\generated\\LibreCAD-Installer.exe"
}

return this