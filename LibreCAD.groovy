def LibreCadExePath()
{
	return "%WORKSPACE%\\LibreCAD.exe"
}
def Build()
{
	bat "%WORKSPACE%\\BuildLibreCAD.bat"
}

def BuildInstaller()
{
	bat "%WORKSPACE%\\scripts\\build-windows.bat"
}
def GetInstallerPath()
{
	return "$WORKSPACE\\generated\\LibreCAD-Installer.exe"
}

return this