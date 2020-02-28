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
	def proNestVersion = "13.1.4"
	def proNestFileDirectory = "\\\\cam-file\\devcommon\\Jenkins\\LibreCad\\Built by ProNest\\" + proNestVersion + "\\"
	bat 'copy "' + proNestFileDirectory + fileName + '" ' + '"%WORKSPACE%\\windows\\"'
}
def PreBuild()
{
	CopyProNestFileToWindowsFolder("DwgReader.dll")
	CopyProNestFileToWindowsFolder("Microsoft.ApplicationInsights.dll")
	CopyProNestFileToWindowsFolder("Telemetry.dll")
	CopyBinaryManagementFile("Win32\\SHP2LFF\\S2FDLL.dll")
	CopyBinaryManagementFile("Win32\\SHP2LFF\\shp2lff.exe")
	CopyBinaryManagementFile("Win32\\SHP2LFF\\s2f.exe")
	CopyProNestFileToWindowsFolder("ProNestUtils.dll")
}
def CopyBinaryManagementFile(pathInBinaryManagement)
{
	def
		binaryManagementFolder = "\\\\cam-file\\Devcommon\\Jenkins\\Binary Management\\"
	def 
		filePath = binaryManagementFolder + pathInBinaryManagement
	bat 'copy "' + filePath + '" ' + '"%WORKSPACE%\\windows\\"'
}
def BuildInstaller()
{
	PreBuild()
	bat "%WORKSPACE%\\BuildLibreCADInstaller.bat"
}
def GetInstallerPath()
{
	return "$WORKSPACE\\generated\\LibreCAD-Installer.exe"
}

return this