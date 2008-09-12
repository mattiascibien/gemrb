#character generation, biography (GUICG23)
import GemRB

BioWindow = 0
EditControl = 0

def OnLoad ():
	global BioWindow, EditControl

	GemRB.LoadWindowPack ("GUICG", 640, 480)
	BioWindow = GemRB.LoadWindow (23)

	EditControl = GemRB.GetControl (BioWindow, 3)
	GemRB.SetText (BioWindow, EditControl, 53605)

	OkButton = GemRB.GetControl (BioWindow, 1)
	GemRB.SetText (BioWindow, OkButton, 53604)

	ClearButton = GemRB.GetControl (BioWindow,2)
	GemRB.SetText (BioWindow, ClearButton, 53602)

	CancelButton = GemRB.GetControl (BioWindow,4)
	GemRB.SetText (BioWindow, CancelButton, 13727)

	GemRB.SetEvent (BioWindow, OkButton, IE_GUI_BUTTON_ON_PRESS, "OkPress")
	GemRB.SetEvent (BioWindow, ClearButton, IE_GUI_BUTTON_ON_PRESS, "ClearPress")
	GemRB.SetEvent (BioWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetVisible (BioWindow,1)
	return

def OkPress ():
	global BioWindow, EditControl

	BioData = GemRB.QueryText (BioWindow, EditControl)
	GemRB.UnloadWindow (BioWindow)
	GemRB.SetNextScript ("CharGen9")
	GemRB.SetToken ("BIO", BioData)
	return
	
def CancelPress ():
	GemRB.UnloadWindow (BioWindow)
	GemRB.SetNextScript ("CharGen9")
	return

def ClearPress ():
        GemRB.SetToken ("BIO", "")
        GemRB.SetText (BioWindow, EditControl, GemRB.GetToken ("BIO") )
        return
