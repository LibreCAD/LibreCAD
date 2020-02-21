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
	bat "mkdir %WORKSPACE%\\generated\\"
	writeFile file: GetInstallerPath(), text: "something"
}
def GetInstallerPath()
{
	return "$WORKSPACE\\generated\\LibreCAD-Installer.exe"
}

return this