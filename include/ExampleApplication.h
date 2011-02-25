/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2006 Torus Knot Software Ltd
Also see acknowledgements in Readme.html

You may use this sample code for anything you like, it is not covered by the
LGPL like the rest of the engine.
-----------------------------------------------------------------------------
*/
/*
-----------------------------------------------------------------------------
Filename:    ExampleApplication.h
Description: Base class for all the OGRE examples
-----------------------------------------------------------------------------
*/

#ifndef __ExampleApplication_H__
#define __ExampleApplication_H__

#include "Ogre.h"
#include "OgreConfigFile.h"
#include "Globals.h"
#include "ExampleFrameListener.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <shlwapi.h>
#include <direct.h>
//#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
//#include <sys/stat.h>
#endif
#include <sys/stat.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#include <CoreFoundation/CoreFoundation.h>

// This function will locate the path to our application on OS X,
// unlike windows you can not rely on the curent working directory
// for locating your configuration files and resources.
std::string macBundlePath()
{
    char path[1024];
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    assert(mainBundle);

    CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
    assert(mainBundleURL);

    CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
    assert(cfStringRef);

    CFStringGetCString(cfStringRef, path, 1024, kCFStringEncodingASCII);

    CFRelease(mainBundleURL);
    CFRelease(cfStringRef);

    return std::string(path);
}
#endif

using namespace Ogre;

/** Base class which manages the standard startup of an Ogre application.
    Designed to be subclassed for specific examples if required.
*/
class ExampleApplication
{
public:
    /// Standard constructor
    ExampleApplication()
    {
        mFrameListener = 0;
        mRoot = 0;
		// Provide a nice cross platform solution for locating the configuration files
		// On windows files are searched for in the current working directory, on OS X however
		// you must provide the full path, the helper function macBundlePath does this for us.
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
		mResourcePath = macBundlePath() + "/Contents/Resources/";
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX //Actually this can be other things than linux as well
//Get path of data
		char* path = std::getenv("OPENDUNGEONS_DATA_PATH");
		if(path)
		{
			mResourcePath = path;
			if(*mResourcePath.end() != '/') //Make sure we have trailing slash
			{
				mResourcePath.append("/");
			}
			//Getenv return value should not be touched/freed.
		}
		else
		{
			mResourcePath = "";
		}
#else
		mResourcePath = "";
#endif
		mHomePath = getHomePath();
    }
    /// Standard destructor
    virtual ~ExampleApplication()
    {
        if (mFrameListener)
            delete mFrameListener;
        if (mRoot)
            delete mRoot;
    }

    /// Start the example
    virtual void go(void)
    {
        if (!setup())
            return;

        mRoot->startRendering();

        // clean up
        destroyScene();
    }

protected:
    Root *mRoot;
    Camera* mCamera;
    ExampleFrameListener* mFrameListener;
    RenderWindow* mWindow;
	Ogre::String mResourcePath;
	Ogre::String mHomePath;

    // These internal methods package up the stages in the startup process
    /** Sets up the application - returns false if the user chooses to abandon configuration. */
    virtual bool setup(void)
    {

		String pluginsPath;
		// only use plugins.cfg if not static
#ifndef OGRE_STATIC_LIB
		pluginsPath = mResourcePath + "plugins.cfg";
#endif
		
        mRoot = new Root(pluginsPath, mHomePath + "ogre.cfg", mHomePath + "Ogre.log");

        setupResources();

        bool carryOn = configure();
        if (!carryOn) return false;

        chooseSceneManager();
        createCamera();
        createViewports();

        // Set default mipmap level (NB some APIs ignore this)
        TextureManager::getSingleton().setDefaultNumMipmaps(5);

		// Create any resource listeners (for loading screens)
		createResourceListener();
		// Load resources
		loadResources();

	createScene();

        createFrameListener();

        return true;

    }

    /** Configures the application - returns false if the user chooses to abandon configuration. */
    virtual bool configure(void)
    {
        // Show the configuration dialog and initialise the system
        // You can skip this and use root.restoreConfig() to load configuration
        // settings if you were sure there are valid ones saved in ogre.cfg
        if(mRoot->showConfigDialog())
        {
            // If returned true, user clicked OK so initialise
            // Here we choose to let the system create a default rendering window by passing 'true'
            mWindow = mRoot->initialise(true);
            return true;
        }
        else
        {
            return false;
        }
    }

    virtual void chooseSceneManager(void)
    {
        // Create the SceneManager, in this case a generic one
        mSceneMgr = mRoot->createSceneManager(ST_GENERIC, "SceneManager");
    }

    virtual void createCamera(void)
    {
        // Create the camera
        mCamera = mSceneMgr->createCamera("PlayerCam");

        // Position it at 500 in Z direction
        mCamera->setPosition(Vector3(0,0,500));
        // Look back along -Z
        mCamera->lookAt(Vector3(0,0,-300));
        mCamera->setNearClipDistance(5);

    }

    virtual void createFrameListener(void)
    {
        mFrameListener = new ExampleFrameListener(mWindow, mCamera, NULL, NULL, false, false, false);
        mFrameListener->showDebugOverlay(true);
        mRoot->addFrameListener(mFrameListener);
    }

    virtual void createScene(void) = 0;    // pure virtual - this has to be overridden

    virtual void destroyScene(void){}    // Optional to override this

    virtual void createViewports(void)
    {
        // Create one viewport, entire window
        Viewport* vp = mWindow->addViewport(mCamera);
        vp->setBackgroundColour(ColourValue(0,0,0));

        // Alter the camera aspect ratio to match the viewport
        mCamera->setAspectRatio(
            Real(vp->getActualWidth()) / Real(vp->getActualHeight()));
    }

    /// Method which will define the source of resources (other than current folder)
    virtual void setupResources(void)
    {
        // Load resource paths from config file
        ConfigFile cf;
        cf.load(mResourcePath + "resources.cfg");

        // Go through all sections & settings in the file
        ConfigFile::SectionIterator seci = cf.getSectionIterator();

        String secName, typeName, archName;
        while (seci.hasMoreElements())
        {
            secName = seci.peekNextKey();
            ConfigFile::SettingsMultiMap *settings = seci.getNext();
            ConfigFile::SettingsMultiMap::iterator i;
            for (i = settings->begin(); i != settings->end(); ++i)
            {
                typeName = i->first;
                //Prefix resource path to resource locations.
                archName = mResourcePath + i->second;
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
                // OS X does not set the working directory relative to the app,
                // In order to make things portable on OS X we need to provide
                // the loading with it's own bundle path location
                ResourceGroupManager::getSingleton().addResourceLocation(
                    String(macBundlePath() + "/" + archName), typeName, secName);
#else
                ResourceGroupManager::getSingleton().addResourceLocation(
                    archName, typeName, secName);
#endif
            }
        }
    }

	/// Optional override method where you can create resource listeners (e.g. for loading screens)
	virtual void createResourceListener(void)
	{

	}

	/// Optional override method where you can perform resource group loading
	/// Must at least do ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
	virtual void loadResources(void)
	{
		// Initialise, parse scripts etc
		ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

	}

	Ogre::String getHomePath()
	{

	    Ogre::String homePath;

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	    //Currently using run dir in windows.
/*	    TCHAR szPath[MAX_PATH];
	    HRESULT result = SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath);
	    if(SUCCEEDED(result))
	    {
	        PathAppend(szPath, _T("\\OpenDungeons"));


	        int len = _tcslen(szPath);
	        char* sZto = new char[len];
	        //TODO - check what encoding to use.
	        WideCharToMultiByte(CP_UTF8, 0, szPath, -1, sZto, len, NULL, NULL);

	        homePath = sZto;

	        _stat statbuf;
	        int status = stat(homePath.c_str(), &statbuf);
            if(status != 0)
            {
                int dirCreateStatus;
                dirCreateStatus = mkdir(homePath.c_str());
            }

            delete[] sZto;
            homePath.append("\\");
	    }
        else
        {
            homePath = ".";
        }*/
	    homePath = "./";
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX

	    //If variable is not set, assume we are in a build dir and
	    //use the current dir for config files.
	    char* useHomeDir = std::getenv("OPENDUNGEONS_DATA_PATH");
	    if(useHomeDir)
	    {
            //On linux and similar, use home dir
            //http://linux.die.net/man/3/getpwuid
            char* path = std::getenv("HOME");
            if(path)
            {
                homePath = path;
            }
            else //In the unlikely event that home is not defined, use current  working dir
            {
                homePath = ".";
            }
            homePath.append("/.OpenDungeons");

            //Create directory if it doesn't exist
            struct stat statbuf;
            int status = stat(homePath.c_str(), &statbuf);
            if(status != 0)
            {
                int dirCreateStatus;
                dirCreateStatus = mkdir(homePath.c_str(),
                        S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

            }

            homePath.append("/");
	    }
	    else
	    {
	        homePath = "./";
	    }
#else
	    homePath = "./";
#endif


	    return homePath;
	}

};


#endif
