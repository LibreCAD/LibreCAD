def LibreCadExePath()
{
	return "%WORKSPACE%\\LibreCAD.exe"
}
def Build()
{
	writefile(LibreCadExePath(),"")
	return this
}
def GetBuiltFiles()
{
	return LibreCadExePath()
}
def BuildInstaller()
{
}
def GetInstallerPath()
{
	return "%WORKSPACE%\\generated\\LibreCAD-Installer.exe"
}