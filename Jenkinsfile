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
		  }
		}
	
		stage('Deploy') 
		{
		steps 
		  {
			bat 'SET'
		  }
		}
		stage('Email') 
		{
			steps 
		  {
			bat 'SET'
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