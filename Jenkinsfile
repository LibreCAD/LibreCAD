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