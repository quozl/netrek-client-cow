/*
   spopt.h
   
   Functions to look after the Short Packet window.
*/   


void sprefresh(int i);
/*
   Refresh button i in the Short Packed Window.
   
   Buttons are:
     SPK_VFIELD		- Variable short packets.
     SPK_MFIELD		- Messages.
     SPK_KFIELD		- Kill Messages.
     SPK_WFIELD		- Warn Messages.
     SPK_TFIELD		- Recieve Threshold.
     SPK_DONE		- Done.
*/


void spaction(W_Event * data);
/*
   Handle a button press.
*/


void spwindow(void);
/*
   Display the Short Packet window.
*/


void spdone(void);
/*
   Unmap the Short Packet window.
*/


