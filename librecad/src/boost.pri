
# Necessary Boost libraries, e. g. BOOST_LIBS = iostreams math_c99 regex
# XXX: this will not play nice with the win case as-is

exists( custom.pro ):include( custom.pro )
exists( custom.pri ):include( custom.pri )

BOOST_LIBS =

unix {
    defineTest( checkBoostIncDir ) {

        boostIncDir = $${1}

        exists( $${boostIncDir}/boost/version.hpp ) {
            return( true )
        }

        return(false)
    }

    defineTest( checkBoostLibDir ) {

        boostLibDir = $${1}

        # If no Boost libs are used, always succeed
        isEmpty( BOOST_LIBS ) {
            return( true )
        }

        exists( $${boostLibDir}/libboost_* ) {
            return( true )
        }

        return( false )
    }

    defineTest( checkBoostLib ) {

        boostLib = $${1}

        exists( $${BOOST_LIBDIR}/libboost_$${boostLib}.* ) {
            return( true )
        }

        return( false )
    }

    defineTest( checkBoostLibs ) {

        boostLibs = $${ARGS}

        # If no Boost libs are used, always succeed
        isEmpty( BOOST_LIBS ) {
            return( true )
        }

        for( boostLib, boostLibs ) {
            checkBoostLib( $${boostLib} ) {
                return( true )
            } else {
                error( Required Boost library $${boostLib} not found )
            }
        }

        return( false )
    }

    defineReplace( findBoostDirIn ) {

        boostDirs = $${ARGS}

        for( boostDir, boostDirs ) {
            checkBoostIncDir( $${boostDir}/include ) : checkBoostLibDir( $${boostDir}/lib ) {
                return( $${boostDir} )
            }
        }

        return( )
    }

    isEmpty( BOOST_DIR ) {

        BOOST_DIR = $$findBoostDirIn($${INSTALL_PREFIX} /usr /usr/local /usr/pkg /opt/local /opt/homebrew /opt/homebrew/Cellar )

        isEmpty( BOOST_DIR ) {
            error( Boost installation not found. )
        } else {
            message( Found Boost in $${BOOST_DIR} )
        }

    }

    checkBoostIncDir( $${BOOST_DIR}/include ) {
        BOOST_INCDIR = $${BOOST_DIR}/include
        message( Using Boost includes from $${BOOST_INCDIR} )
    } else {
        error( $${BOOST_DIR} does not contain a Boost installation )
    }

    !isEmpty( BOOST_LIBS ) {

        checkBoostLibDir( $${BOOST_DIR}/lib ) {
            BOOST_LIBDIR = $${BOOST_DIR}/lib
        } else {
            error( Boost libraries not installed in $${BOOST_DIR}/lib )
        }

        !checkBoostLibs( $${BOOST_LIBS} ) {
            # NOTREACHED
            error( Required Boost libraries not found )
        }
    }


    #BOOST_INCDIR -= $$QMAKE_DEFAULT_INCDIRS
    INCLUDEPATH += $${BOOST_INCDIR}
    HEADERS += $${BOOST_INCDIR}

    !isEmpty( BOOST_LIBS ) {

        LIBS += -L$${BOOST_LIBDIR}

        message( Using Boost libraries from $${BOOST_LIBDIR} )
        message( Using Boost libraries: )

        for( boostLib, BOOST_LIBS ) {
            message( -> $${boostLib} )
            LIBS += -lboost_$${boostLib}
        }
    }

}

win32 {
	!equals($$(BOOST_DIR), ""):exists( "$$(BOOST_DIR)" ) {
		# BOOST_DIR environment variable is set, use it:
		BOOST_DIR = "$$(BOOST_DIR)"
	} else:isEmpty( BOOST_DIR ) {
		# BOOST_DIR QMake variable is not set at all (in custom.pro), use a hardcoded default:
		BOOST_DIR = "/boost/boost_1_53_0"
    }

    !exists( "$${BOOST_DIR}/boost/version.hpp" ) {
        error( "Can not find Boost installation in $${BOOST_DIR}" )
    }

    BOOST_INCDIR = "$${BOOST_DIR}"

    INCLUDEPATH += "$${BOOST_INCDIR}"
    HEADERS += "$${BOOST_INCDIR}"

}

