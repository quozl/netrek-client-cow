/*******************************************************************************
 * Simple LiveConnect Sample Plugin
 * Copyright (c) 1996 Netscape Communications. All rights reserved.
 ******************************************************************************/

import netscape.plugin.Plugin;

class COW extends Plugin {

    /*
    ** This native method will give us access to the netrek window.
    */

    public native void run(String server, int port);

    /*
    ** This is a publically callable new feature that our plug-in is
    ** providing. We can call it from JavaScript, Java, or from native
    ** code.
    */
    public void connect(String Server) {
      run(Server,-1);   // Use the .xtrekrc defaults
    }

}

