pipeline 
{
	agent any
	stages 
	{
		stage('Checkout') 
		{
		  steps 
		  {
			bat 'SET'
			deleteDir()
			checkout scm
			stash name: 'source'
		  }
		}
		stage('Build') 
		{
			steps 
			{
				bat 'SET'
				script
				{
					def LibreCAD = load 'LibreCAD.groovy'
					LibreCAD.Build()
					def builtFiles = LibreCAD.GetBuiltFiles()
					stash name: 'build_files'
				}
			}
			post
			{
				always
				{
					archiveArtifacts allowEmptyArchive: true, artifacts: 'LibreCad.exe'
				}
			}
		}
	
		stage('Deploy') 
		{
			steps 
			{
				bat 'SET'
				// build installer
				unstash 'build_files'
				// copy installer to prod rel
				def LibreCAD = load 'LibreCAD.groovy'
				LibreCAD.BuildInstaller()
				bat script: 'copy "' +LibreCAD.GetInstallerPath()+ '" "\\\\cam-issvr\\prodrel\\WIN_Prod\\CAD\\LibreCAD" -y'
				
				script
				{
					// create installer link file
					CreateInstallerLinksFile("InstallerLinksLibreCAD.txt", "\\\\cam-issvr\\prodrel\\WIN_Prod\\CAD\\LibreCAD\\LibreCAD-Installer.exe")
				}
			}
		}
		stage('Email') 
		{
			steps 
			{
				bat 'SET'
				deleteDir()
				unstash 'source'
				script 
				{
					def notify = load 'notify.groovy'
					if (!env.INSTALLER_TYPE || (env.INSTALLER_TYPE == '')) 
					{
						notify.sendSuccessEmail(false)
					} 
					else 
					{
						unstash 'installer'
						notify.sendSuccessEmail(true)
					}          
				}
			}
			post 
			{	
				always 
				{
					script 
					{
						if (mustCleanWorkspace()) 
						{
							deleteDir()
						}
					}
				}
		  
			}
        }
	}
	post 
	{
	    failure 
		{
			emailext(subject: "${env.PRODUCT_NAME} ${env.VERSION_FULL} - Failure!",
				body: """<p>Check the results for <a href='${currentBuild.absoluteUrl}'>Build #${currentBuild.number}</a>.</p>""",
				mimeType: 'text/html',
				recipientProviders: [[$class: 'DevelopersRecipientProvider']])   
		}
	}
	environment 
	{
		PRODUCT_NAME = 'LibreCAD for ProNest'
		VERSION_BUILD = getVersionBuild()
		INSTALLER_TYPE = getInstallerType()
		RECIPIENTS = 'mtcprogramming, steven.bertken, chris.pollard'
		VERSION_FULL = ''
	}
	options 
	{
		buildDiscarder(logRotator(numToKeepStr: '30'))
	}
}
def mustCleanWorkspace()
{
	return false
}

def getVersionBuild()  
{    
  return new Date() - new Date("1/1/2000")
}

def getInstallerType() 
{
	return "Release"
}
def CreateInstallerLinksFile(linksFileName, pathToInstaller, version)
{
	def body = "<p><a href='" + pathToInstaller + "'>LibreCAD" +  version + "</a>"
	writefile(linksFileName, body);
}