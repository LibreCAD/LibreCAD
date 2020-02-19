def sendSuccessEmail(includeInstallerLinks) {
  // Default values
  def installer = ""
  if (env.INSTALLER_TYPE) {
    installer = env.INSTALLER_TYPE
  } else {
    installer = "(bootleg)"
  }
  
  def subject = "${env.PRODUCT_NAME} ${env.VERSION_FULL} " + installer + " - Success!"
  def body = """<h3>Build information</h3>
    <table>
      <tr>
        <td>Jenkins project:</td>
        <td><a href="${env.JOB_DISPLAY_URL}">${env.PRODUCT_NAME}</a></td>
      </tr>
      <tr>
        <td>Branch:</td>
        <td>${env.BRANCH_NAME}</td>
      </tr>
      <tr>
        <td>Version:</td>
        <td>${env.VERSION_FULL}</td>
      </tr>
      <tr>
        <td>Modifier:</td>
        <td>""" + installer + """</td>
      </tr>
      <tr>
        <td>Build results:</td>
        <td><a href="${env.RUN_DISPLAY_URL}">Build ${env.BUILD_DISPLAY_NAME}</a></td>
      </tr>
    </table>"""

  if (includeInstallerLinks) {
    // Create section for 64-bit installer links...
    body = body + """
      <h3>64-bit Installers</h3>"""

    if (fileExists("InstallerLinksPN64.txt")) {
      body = body + readFile("InstallerLinksPN64.txt")
    }

    // Create section for 32-bit installer links...

    body = body + """
      <h3>32-bit Installers</h3>"""
	
	if(fileExists("InstallerLinksLibreCad.txt"))
		body = body + readFile("InstallerLinksLibreCad.txt")
	
    body = body + """
      <br/>
      <p><i>This document is for internal use only. Do not distribute outside of Hypertherm.</i></p>"""

    emailext (
      subject: subject,
      body: body,
      mimeType: 'text/html',
      to: "${env.RECIPIENTS}"
    ) 
  } else {
    emailext (
      subject: subject,
      body: body,
      mimeType: 'text/html',
      recipientProviders: [[$class: 'DevelopersRecipientProvider']]
    )
  }
}

return this
