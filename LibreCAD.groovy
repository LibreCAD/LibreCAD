def LibreCadExePath()
{
	return "%WORKSPACE%\\LibreCAD.exe"
}
def Build()
{
	writeFile file: LibreCadExePath(), text: "something"
}
def GetBuiltFiles()
{
	return LibreCadExePath()
}
def BuildInstaller()
{
	bat "mkdir %WORKSPACE%\\generated\\"
	writeFile file: GetInstallerPath(), text: "something"
}
def GetInstallerPath()
{
	return "%WORKSPACE%\\generated\\LibreCAD-Installer.exe"
}

return this