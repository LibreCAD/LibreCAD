<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>NSPrincipalClass</key>
	<string>NSApplication</string>
	<key>NSHighResolutionCapable</key>
	<true/>
	<key>CFBundleIconFile</key>
	<string>@ICON@</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleGetInfoString</key>
	<string>LibreCAD @FULL_VERSION@</string>
	<key>CFBundleShortVersionString</key>
	<string>@FULL_VERSION@</string>
	<key>CFBundleVersion</key>
	<string>@FULL_VERSION@</string>
	<key>CFBundleSignature</key>
	<string>@TYPEINFO@</string>
	<key>CFBundleExecutable</key>
	<string>@EXECUTABLE@</string>
	<!-- Not a @TOKEN@: qmake derives the identifier from TARGET, which would
	     give org.librecad.LibreCAD, while the CMake build sets it directly.
	     Spelling it out here keeps both builds on one identifier. -->
	<key>CFBundleIdentifier</key>
	<string>org.librecad.librecad</string>
	<key>CFBundleDocumentTypes</key>
	<array>
		<dict>
			<key>CFBundleTypeExtensions</key>
			<array>
				<string>dxf</string>
				<string>llf</string>
				<string>cxf</string>
			</array>
			<key>CFBundleTypeRole</key>
			<string>Editor</string>
		</dict>
		<dict>
			<key>CFBundleTypeExtensions</key>
			<array>
				<string>dwg</string>
			</array>
			<key>CFBundleTypeRole</key>
			<string>Viewer</string>
		</dict>
	</array>
</dict>
</plist>
