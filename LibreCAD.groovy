def LibreCadExePath()
{
	return "%WORKSPACE%\\LibreCAD.exe"
}
def Build()
{
	bat "%WORKSPACE%\\BuildLibreCAD.bat"
}
def CopyProNestFileToWindowsFolder(fileName)
{
	def proNestVersion = 13.1.4
	def proNestFileDirectory = "\\\\cam-file\\devcommon\\Jenkins\\LibreCad\\Built by ProNest\\" + proNestVersionb + "\\"
	bat 'copy "' + proNestFileDirectory + fileName + '" ' + '"%WORKSPACE%\\windows\\"'
}
def PreBuild()
{
	CopyProNestFileToWindowsFolder("DwgReader.dll")
	CopyProNestFileToWindowsFolder("Microsoft.ApplicationInsights.dll")
	CopyProNestFileToWindowsFolder("Telemetry.dll")
}
def BuildInstaller()
{
	PreBuild()
	bat "cd scripts && %WORKSPACE%\\scripts\\build-windows.bat"
}
def GetInstallerPath()
{
	return "$WORKSPACE\\generated\\LibreCAD-Installer.exe"
}

return this