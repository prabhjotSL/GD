#include "SceneCanvas.h"
#include <string>
#include <iostream>
#include <vector>
#include <cmath>
#include <sstream>
#include <wx/config.h>
#include <wx/cursor.h>
#include <wx/log.h>
#include <wx/scrolbar.h>
#ifdef __WXMSW__
#include <wx/msw/winundef.h>
#endif
#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include "GDL/RuntimeScene.h"
#include "GDL/Chercher.h"
#include "GDL/ExtensionsManager.h"
#include "GDL/ImageManager.h"
#include "GDL/RuntimeGame.h"
#include "GDL/Object.h"
#include "GDL/Collisions.h"
#include "GDL/Event.h"
#include "GDL/Chercher.h"
#include "GDL/CommonTools.h"
#include "GDL/HelpFileAccess.h"
#include "GDL/ChooseLayer.h"
#include "GDL/ChooseObject.h"
#include "EditOptionsPosition.h"
#include "Clipboard.h"
#include "DndTextSceneEditor.h"
#include "InitialPositionBrowserDlg.h"
#include "RenderDialog.h"
#include "EditorObjets.h"
#include "EditorLayers.h"
#include "DebuggerGUI.h"
#include "GridSetup.h"

const long SceneCanvas::ID_ADDOBJMENU = wxNewId();
const long SceneCanvas::ID_DELOBJMENU = wxNewId();
const long SceneCanvas::ID_PROPMENU = wxNewId();
const long SceneCanvas::ID_LAYERUPMENU = wxNewId();
const long SceneCanvas::ID_LAYERDOWNMENU = wxNewId();
const long SceneCanvas::ID_COPYMENU = wxNewId();
const long SceneCanvas::ID_CUTMENU = wxNewId();
const long SceneCanvas::ID_PASTEMENU = wxNewId();
const long SceneCanvas::idRibbonEditMode = wxNewId();
const long SceneCanvas::idRibbonPreviewMode = wxNewId();

const long SceneCanvas::idRibbonObjectsEditor = wxNewId();
const long SceneCanvas::idRibbonLayersEditor = wxNewId();
const long SceneCanvas::idRibbonChooseObject = wxNewId();
const long SceneCanvas::idRibbonOrigine = wxNewId();
const long SceneCanvas::idRibbonOriginalZoom = wxNewId();
const long SceneCanvas::idRibbonGrid = wxNewId();
const long SceneCanvas::idRibbonWindowMask = wxNewId();
const long SceneCanvas::idRibbonGridSetup = wxNewId();

const long SceneCanvas::idRibbonRefresh = wxNewId();
const long SceneCanvas::idRibbonPlay = wxNewId();
const long SceneCanvas::idRibbonPlayWin = wxNewId();
const long SceneCanvas::idRibbonPause = wxNewId();
const long SceneCanvas::idRibbonResetGlobalVars = wxNewId();
const long SceneCanvas::idRibbonDebugger = wxNewId();

const long SceneCanvas::idRibbonUndo = wxNewId();
const long SceneCanvas::idRibbonRedo = wxNewId();

const long SceneCanvas::idRibbonHelp = wxNewId();


SceneCanvas::SceneCanvas( wxWindow* Parent, RuntimeGame & game_, Scene & scene_, MainEditorCommand & mainEditorCommand_, wxWindowID Id, const wxPoint& Position, const wxSize& Size, long Style ) :
        wxSFMLCanvas( Parent, Id, Position, Size, Style ),
        gameEdited(game_),
        sceneEdited(scene_),
        game(gameEdited),
        scene(this, &game),
        hasJustRightClicked(false),
        mainEditorCommand( mainEditorCommand_ ),
        scrollBar1(NULL),
        scrollBar2(NULL)
{
    MemTracer.AddObj( "Editeur de sc�ne", ( long )this );

    SetView( scene.view );
    SetFramerateLimit( gameEdited.maxFPS );
    UseVerticalSync( gameEdited.verticalSync );
    Clear( sf::Color( 125, 125, 125, 255 ) );

    Connect(ID_ADDOBJMENU,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&SceneCanvas::OnAddObjetSelected);
    Connect(ID_DELOBJMENU,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&SceneCanvas::OnDelObjetSelected);
    Connect(ID_PROPMENU,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&SceneCanvas::OnPropObjSelected);
    Connect(ID_LAYERUPMENU,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&SceneCanvas::OnLayerUpSelected);
    Connect(ID_LAYERDOWNMENU,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&SceneCanvas::OnLayerDownSelected);
    Connect(ID_COPYMENU,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&SceneCanvas::OnCopySelected);
    Connect(ID_CUTMENU,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&SceneCanvas::OnCutSelected);
    Connect(ID_PASTEMENU,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&SceneCanvas::OnPasteSelected);

    //Generate context menu
    wxMenuItem * layerUpItem = new wxMenuItem((&contextMenu), ID_LAYERUPMENU, _("Passer le(s) objet(s) sur le calque sup�rieur"), wxEmptyString, wxITEM_NORMAL);
    layerUpItem->SetBitmap(wxImage( "res/up.png" ) );
    wxMenuItem * layerDownItem = new wxMenuItem((&contextMenu), ID_LAYERDOWNMENU, _("Passer le(s) objet(s) sur le calque inf�rieur"), wxEmptyString, wxITEM_NORMAL);
    layerDownItem->SetBitmap(wxImage( "res/down.png" ) );
    wxMenuItem * deleteItem = new wxMenuItem((&contextMenu), ID_DELOBJMENU, _("Supprimer la s�lection\tDEL"), wxEmptyString, wxITEM_NORMAL);
    deleteItem->SetBitmap(wxImage( "res/deleteicon.png" ) );
    wxMenuItem * addItem = new wxMenuItem((&contextMenu), ID_ADDOBJMENU, _("Ajouter un objet\tINSER"), wxEmptyString, wxITEM_NORMAL);
    addItem->SetBitmap(wxImage( "res/addobjet.png" ) );

    contextMenu.Append(ID_PROPMENU, _("Propri�t�s"));
    contextMenu.AppendSeparator();
    contextMenu.Append(addItem);
    contextMenu.Append(deleteItem);
    contextMenu.AppendSeparator();
    contextMenu.Append(layerUpItem);
    contextMenu.Append(layerDownItem);
    contextMenu.AppendSeparator();
    {
        wxMenuItem * copyItem = new wxMenuItem((&contextMenu), ID_COPYMENU, _("Copier"), wxEmptyString, wxITEM_NORMAL);
        copyItem->SetBitmap(wxImage( "res/copyicon.png" ) );
        contextMenu.Append(copyItem);
        wxMenuItem * cutItem = new wxMenuItem((&contextMenu), ID_CUTMENU, _("Couper"), wxEmptyString, wxITEM_NORMAL);
        cutItem->SetBitmap(wxImage( "res/cuticon.png" ) );
        contextMenu.Append(cutItem);
        wxMenuItem * pasteItem = new wxMenuItem((&contextMenu), ID_PASTEMENU, _("Coller"), wxEmptyString, wxITEM_NORMAL);
        pasteItem->SetBitmap(wxImage( "res/pasteicon.png" ) );
        contextMenu.Append(pasteItem);
    }

    //Generate "no object" context menu
    {
        wxMenuItem * addItem = new wxMenuItem((&noObjectContextMenu), ID_ADDOBJMENU, _("Ajouter un objet\tINSER"), wxEmptyString, wxITEM_NORMAL);
        addItem->SetBitmap(wxImage( "res/addobjet.png" ) );
        noObjectContextMenu.Append(addItem);
        noObjectContextMenu.AppendSeparator();
        wxMenuItem * pasteItem = new wxMenuItem((&noObjectContextMenu), ID_PASTEMENU, _("Coller"), wxEmptyString, wxITEM_NORMAL);
        pasteItem->SetBitmap(wxImage( "res/pasteicon.png" ) );
        noObjectContextMenu.Append(pasteItem);
    }

    SetDropTarget(new DndTextSceneEditor(*this));

    CreateToolsBar(mainEditorCommand.GetRibbonSceneEditorButtonBar(), scene.editing);
}

void SceneCanvas::ConnectEvents()
{
    mainEditorCommand.GetMainEditor()->Connect(idRibbonEditMode, wxEVT_COMMAND_RIBBONBUTTON_CLICKED, (wxObjectEventFunction)&SceneCanvas::OnEditionBtClick, NULL, this);
    mainEditorCommand.GetMainEditor()->Connect(idRibbonPreviewMode, wxEVT_COMMAND_RIBBONBUTTON_CLICKED, (wxObjectEventFunction)&SceneCanvas::OnPreviewBtClick, NULL, this);

    mainEditorCommand.GetMainEditor()->Connect(idRibbonObjectsEditor, wxEVT_COMMAND_RIBBONBUTTON_CLICKED, (wxObjectEventFunction)&SceneCanvas::OnObjectsEditor, NULL, this);
    mainEditorCommand.GetMainEditor()->Connect(idRibbonLayersEditor, wxEVT_COMMAND_RIBBONBUTTON_CLICKED, (wxObjectEventFunction)&SceneCanvas::OnLayersEditor, NULL, this);
    mainEditorCommand.GetMainEditor()->Connect(idRibbonChooseObject, wxEVT_COMMAND_RIBBONBUTTON_CLICKED, (wxObjectEventFunction)&SceneCanvas::OnChoisirObjetBtClick, NULL, this);
    mainEditorCommand.GetMainEditor()->Connect(idRibbonOrigine, wxEVT_COMMAND_RIBBONBUTTON_CLICKED, (wxObjectEventFunction)&SceneCanvas::OnOrigineBtClick, NULL, this);
    mainEditorCommand.GetMainEditor()->Connect(idRibbonOriginalZoom, wxEVT_COMMAND_RIBBONBUTTON_CLICKED, (wxObjectEventFunction)&SceneCanvas::OnZoomInitBtClick, NULL, this);
    mainEditorCommand.GetMainEditor()->Connect(idRibbonOriginalZoom, wxEVT_COMMAND_RIBBONBUTTON_DROPDOWN_CLICKED, (wxObjectEventFunction)&SceneCanvas::OnZoomMoreBtClick, NULL, this);
    mainEditorCommand.GetMainEditor()->Connect(idRibbonGrid, wxEVT_COMMAND_RIBBONBUTTON_CLICKED, (wxObjectEventFunction)&SceneCanvas::OnGridBtClick, NULL, this);
    mainEditorCommand.GetMainEditor()->Connect(idRibbonGridSetup, wxEVT_COMMAND_RIBBONBUTTON_CLICKED, (wxObjectEventFunction)&SceneCanvas::OnGridSetupBtClick, NULL, this);
    mainEditorCommand.GetMainEditor()->Connect(idRibbonWindowMask, wxEVT_COMMAND_RIBBONBUTTON_CLICKED, (wxObjectEventFunction)&SceneCanvas::OnWindowMaskBtClick, NULL, this);
    mainEditorCommand.GetMainEditor()->Connect(idRibbonUndo,wxEVT_COMMAND_RIBBONBUTTON_CLICKED,(wxObjectEventFunction)&SceneCanvas::OnUndoBtClick, NULL, this);
    mainEditorCommand.GetMainEditor()->Connect(idRibbonRedo,wxEVT_COMMAND_RIBBONBUTTON_CLICKED,(wxObjectEventFunction)&SceneCanvas::OnRedoBtClick, NULL, this);

    mainEditorCommand.GetMainEditor()->Connect(idRibbonRefresh, wxEVT_COMMAND_RIBBONBUTTON_CLICKED, (wxObjectEventFunction)&SceneCanvas::OnRefreshBtClick, NULL, this);
    mainEditorCommand.GetMainEditor()->Connect(idRibbonPlay, wxEVT_COMMAND_RIBBONBUTTON_CLICKED, (wxObjectEventFunction)&SceneCanvas::OnPlayBtClick, NULL, this);
    mainEditorCommand.GetMainEditor()->Connect(idRibbonPlayWin, wxEVT_COMMAND_RIBBONBUTTON_CLICKED, (wxObjectEventFunction)&SceneCanvas::OnPlayWindowBtClick, NULL, this);
    mainEditorCommand.GetMainEditor()->Connect(idRibbonPause, wxEVT_COMMAND_RIBBONBUTTON_CLICKED, (wxObjectEventFunction)&SceneCanvas::OnPauseBtClick, NULL, this);
    mainEditorCommand.GetMainEditor()->Connect(idRibbonDebugger, wxEVT_COMMAND_RIBBONBUTTON_CLICKED, (wxObjectEventFunction)&SceneCanvas::OnDebugBtClick, NULL, this);
/*
	mainEditorCommand.GetMainEditor()->Connect(ID_MENUITEM8,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&SceneCanvas::Onzoom5Selected, NULL, this);
	mainEditorCommand.GetMainEditor()->Connect(ID_MENUITEM1,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&SceneCanvas::Onzoom10Selected, NULL, this);
	mainEditorCommand.GetMainEditor()->Connect(ID_MENUITEM2,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&SceneCanvas::Onzoom25Selected, NULL, this);
	mainEditorCommand.GetMainEditor()->Connect(ID_MENUITEM3,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&SceneCanvas::Onzoom50Selected, NULL, this);
	mainEditorCommand.GetMainEditor()->Connect(ID_MENUITEM4,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&SceneCanvas::Onzoom100Selected, NULL, this);
	mainEditorCommand.GetMainEditor()->Connect(ID_MENUITEM5,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&SceneCanvas::Onzoom150Selected, NULL, this);
	mainEditorCommand.GetMainEditor()->Connect(ID_MENUITEM6,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&SceneCanvas::Onzoom200Selected, NULL, this);
	mainEditorCommand.GetMainEditor()->Connect(ID_MENUITEM7,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&SceneCanvas::Onzoom500Selected, NULL, this);*/
}

/**
 * Static method for creating the ribbon's page used by Images Editors
 */
wxRibbonButtonBar* SceneCanvas::CreateRibbonPage(wxRibbonPage * page)
{
    bool hideLabels = false;
    wxConfigBase::Get()->Read( _T( "/Skin/HideLabels" ), &hideLabels );

    {
        wxRibbonPanel *ribbonPanel = new wxRibbonPanel(page, wxID_ANY, _("Mode"), wxBitmap("res/view24.png", wxBITMAP_TYPE_ANY), wxDefaultPosition, wxDefaultSize, wxRIBBON_PANEL_DEFAULT_STYLE);
        wxRibbonButtonBar *ribbonBar = new wxRibbonButtonBar(ribbonPanel, wxID_ANY);
        ribbonBar->AddButton(idRibbonEditMode, !hideLabels ? _("Edition") : "", wxBitmap("res/edit24.png", wxBITMAP_TYPE_ANY));
        ribbonBar->AddButton(idRibbonPreviewMode, !hideLabels ? _("Aper�u") : "", wxBitmap("res/view24.png", wxBITMAP_TYPE_ANY));
    }

    wxRibbonPanel *toolsPanel = new wxRibbonPanel(page, wxID_ANY, _("Outils"), wxBitmap("res/tools24.png", wxBITMAP_TYPE_ANY), wxDefaultPosition, wxDefaultSize, wxRIBBON_PANEL_DEFAULT_STYLE);
    wxRibbonButtonBar *toolsBar = new wxRibbonButtonBar(toolsPanel, wxID_ANY);
    CreateToolsBar(toolsBar, true); //Create an initial tool bar

    {
        wxRibbonPanel *ribbonPanel = new wxRibbonPanel(page, wxID_ANY, _("Aide"), wxBitmap("res/helpicon24.png", wxBITMAP_TYPE_ANY), wxDefaultPosition, wxDefaultSize, wxRIBBON_PANEL_DEFAULT_STYLE);
        wxRibbonButtonBar *ribbonBar = new wxRibbonButtonBar(ribbonPanel, wxID_ANY);
        ribbonBar->AddButton(idRibbonHelp, !hideLabels ? _("Aide") : "", wxBitmap("res/helpicon24.png", wxBITMAP_TYPE_ANY));
    }

    return toolsBar; //Returned to the mainEditor, and will then be passed to Scene Editors with MainEditorCommand
}

void SceneCanvas::OnZoomMoreBtClick(wxRibbonButtonBarEvent& evt)
{
    //evt.PopupMenu(&zoomMenu); TODO
}

void SceneCanvas::CreateToolsBar(wxRibbonButtonBar * bar, bool editing)
{
    wxConfigBase *pConfig = wxConfigBase::Get();
    bool hideLabels = false;
    pConfig->Read( _T( "/Skin/HideLabels" ), &hideLabels );

    bar->ClearButtons();

    if ( editing )
    {
        bar->AddButton(idRibbonObjectsEditor, !hideLabels ? _("Editeur d'objets") : "", wxBitmap("res/objeticon24.png", wxBITMAP_TYPE_ANY));
        bar->AddButton(idRibbonLayersEditor, !hideLabels ? _("Editeur de calques") : "", wxBitmap("res/layers24.png", wxBITMAP_TYPE_ANY));
        bar->AddButton(idRibbonChooseObject, !hideLabels ? _("Choisir un objet") : "", wxBitmap("res/addobjet24.png", wxBITMAP_TYPE_ANY));
        bar->AddButton(idRibbonUndo, !hideLabels ? _("Annuler") : "", wxBitmap("res/undo24.png", wxBITMAP_TYPE_ANY));
        bar->AddButton(idRibbonRedo, !hideLabels ? _("Refaire") : "", wxBitmap("res/redo24.png", wxBITMAP_TYPE_ANY));
        bar->AddButton(idRibbonOrigine, !hideLabels ? _("Revenir � l'origine") : "", wxBitmap("res/center24.png", wxBITMAP_TYPE_ANY));
        bar->AddHybridButton(idRibbonOriginalZoom, !hideLabels ? _("Zoom initial") : "", wxBitmap("res/zoom24.png", wxBITMAP_TYPE_ANY));
        bar->AddButton(idRibbonGrid, !hideLabels ? _("Grille") : "", wxBitmap("res/grid24.png", wxBITMAP_TYPE_ANY));
        bar->AddButton(idRibbonGridSetup, !hideLabels ? _("Editer la grille") : "", wxBitmap("res/gridedit24.png", wxBITMAP_TYPE_ANY));
        bar->AddButton(idRibbonWindowMask, !hideLabels ? _("Masque de la fen�tre de jeu") : "", wxBitmap("res/windowMask24.png", wxBITMAP_TYPE_ANY));
    }
    else
    {
        bar->AddButton(idRibbonRefresh, !hideLabels ? _("Rafraichir") : "", wxBitmap("res/refreshicon24.png", wxBITMAP_TYPE_ANY));
        bar->AddButton(idRibbonPlay, !hideLabels ? _("Jouer") : "", wxBitmap("res/starticon24.png", wxBITMAP_TYPE_ANY));
        bar->AddButton(idRibbonPlayWin, !hideLabels ? _("Jouer dans une fen�tre") : "", wxBitmap("res/startwindow24.png", wxBITMAP_TYPE_ANY));
        bar->AddButton(idRibbonPause, !hideLabels ? _("Pause") : "", wxBitmap("res/pauseicon24.png", wxBITMAP_TYPE_ANY));
        bar->AddButton(idRibbonDebugger, !hideLabels ? _("Debugger") : "", wxBitmap("res/bug24.png", wxBITMAP_TYPE_ANY));
    }

    bar->Realize();
}


void SceneCanvas::SetOwnedObjectsEditor(boost::shared_ptr<EditorObjets> objectsEditor_)
{
    objectsEditor = objectsEditor_;
}
void SceneCanvas::SetOwnedLayersEditor(boost::shared_ptr<EditorLayers> layersEditor_)
{
    layersEditor = layersEditor_;
    if ( layersEditor && layersEditor->GetAssociatedSceneCanvas() != this)
        layersEditor->SetAssociatedSceneCanvas(this);
}
void SceneCanvas::SetOwnedDebugger(boost::shared_ptr<DebuggerGUI> debugger_)
{
    debugger = debugger_;
}
void SceneCanvas::SetOwnedExternalWindow(boost::shared_ptr<RenderDialog> externalWindow_)
{
    externalWindow = externalWindow_;
}
void SceneCanvas::SetOwnedInitialPositionBrowser(boost::shared_ptr<InitialPositionBrowserDlg> initialPositionsBrowser_)
{
    initialPositionsBrowser = initialPositionsBrowser_;
}

/**
 * Update the size and position of the canvas displaying the scene
 */
void SceneCanvas::UpdateSize()
{
    if ( scene.editing )
    {
        //Scene takes all the space available in edition mode.
        Window::SetSize(parentPanel->GetSize().GetWidth()-scrollBar2->GetSize().GetWidth(), parentPanel->GetSize().GetHeight()-scrollBar1->GetSize().GetHeight());
        wxWindowBase::SetSize(0,0, parentPanel->GetSize().GetWidth()-scrollBar2->GetSize().GetWidth(), parentPanel->GetSize().GetHeight()-scrollBar1->GetSize().GetHeight());
        scene.view.SetSize(parentPanel->GetSize().GetWidth()-scrollBar2->GetSize().GetWidth(), parentPanel->GetSize().GetHeight()-scrollBar1->GetSize().GetHeight());
    }
    else
    {
        //Scene has the size of the game's window size in preview mode.
        Window::SetSize(game.windowWidth, game.windowHeight);
        wxWindowBase::SetSize(game.windowWidth, game.windowHeight);

        externalWindow->SetSize(game.windowWidth, game.windowHeight);

        //Scene is centered in preview mode
        wxWindowBase::SetSize((parentPanel->GetSize().GetWidth()-wxWindowBase::GetSize().GetX())/2,
                              (parentPanel->GetSize().GetHeight()-wxWindowBase::GetSize().GetY())/2,
                              game.windowWidth, game.windowHeight);
    }
}


/**
 * Go in preview mode
 */
void SceneCanvas::OnPreviewBtClick( wxCommandEvent & event )
{
    scene.editing = false;
    scene.running = false;

    scrollBar1->Show(false);
    scrollBar2->Show(false);
    UpdateSize();

    if ( debugger ) debugger->Play();

    CreateToolsBar(mainEditorCommand.GetRibbonSceneEditorButtonBar(), false);
    mainEditorCommand.GetRibbonSceneEditorButtonBar()->Refresh();
}

/**
 * Go in edition mode
 */
void SceneCanvas::OnEditionBtClick( wxCommandEvent & event )
{
    scene.editing = true;
    scene.running = false;

    scrollBar1->Show(true);
    scrollBar2->Show(true);
    UpdateSize();

    externalWindow->Show(false);

    scene.ChangeRenderWindow(this);

    Reload();
    scene.RenderEdittimeScene(); //FIXME : Hack to make sure OpenGL Rendering is correct

    scene.ChangeRenderWindow(this);

    if ( debugger ) debugger->Pause();

    CreateToolsBar(mainEditorCommand.GetRibbonSceneEditorButtonBar(), true);
    mainEditorCommand.GetRibbonSceneEditorButtonBar()->Refresh();
}


void SceneCanvas::OnHelpBtClick( wxCommandEvent & event )
{
    HelpFileAccess::getInstance()->DisplaySection(12);
}

void SceneCanvas::OnLayersEditor( wxCommandEvent & event )
{
    m_mgr->GetPane(layersEditor.get()).Show();
    m_mgr->Update();
}

void SceneCanvas::OnObjectsEditor( wxCommandEvent & event )
{
    m_mgr->GetPane(objectsEditor.get()).Show();
    m_mgr->Update();
}

void SceneCanvas::OnRefreshBtClick( wxCommandEvent & event )
{
    scene.editing = false;
    scene.running = false;

    Reload();
}

void SceneCanvas::OnZoomInitBtClick( wxCommandEvent & event )
{
    scene.view.SetSize(GetWidth(), GetHeight());
}

////////////////////////////////////////////////////////////
/// Retour aux coordonn�es 0;0 de la sc�ne
////////////////////////////////////////////////////////////
void SceneCanvas::OnOrigineBtClick(wxCommandEvent & event )
{
    scene.view.SetCenter( (game.windowWidth/2),(game.windowHeight/2));
}


////////////////////////////////////////////////////////////
/// Choisir un objet � ajouter
////////////////////////////////////////////////////////////
void SceneCanvas::OnChoisirObjetBtClick( wxCommandEvent & event )
{
    ChooseObject Dialog( this, game, scene, false );
    if ( Dialog.ShowModal() == 1 )
    {
        scene.objectToAdd = Dialog.objectChosen;
    }
}

////////////////////////////////////////////////////////////
/// Activer/Desactiver la grille
////////////////////////////////////////////////////////////
void SceneCanvas::OnGridBtClick( wxCommandEvent & event )
{
    scene.grid = !scene.grid;
}

void SceneCanvas::OnWindowMaskBtClick( wxCommandEvent & event )
{
    scene.windowMask = !scene.windowMask;
}

void SceneCanvas::OnUndoBtClick( wxCommandEvent & event )
{
    if ( history.size() < 2 )
        return;

    redoHistory.push_back(sceneEdited.initialObjectsPositions); //On pourra revenir � l'�tat actuel avec "Refaire"
    sceneEdited.initialObjectsPositions = history.at( history.size() - 2 ); //-2 car le dernier �l�ment est la liste d'�v�nement actuelle
    history.pop_back();

    Reload();
}

void SceneCanvas::OnRedoBtClick( wxCommandEvent & event )
{
    if ( redoHistory.empty() )
        return;

    history.push_back(redoHistory.back()); //Le dernier �l�ment est la liste d'�v�nement actuellement �dit�e
    sceneEdited.initialObjectsPositions = redoHistory.back();
    redoHistory.pop_back();

    Reload();
}

////////////////////////////////////////////////////////////
/// Activer/Desactiver la grille
////////////////////////////////////////////////////////////
void SceneCanvas::OnGridSetupBtClick( wxCommandEvent & event )
{
    GridSetup dialog(this, scene.gridWidth, scene.gridHeight, scene.snap, scene.gridR, scene.gridG, scene.gridB);
    dialog.ShowModal();
}

/**
 * Test scene in editor
 */
void SceneCanvas::OnPlayBtClick( wxCommandEvent & event )
{
    scene.running = true;
    scene.editing = false;

    externalWindow->Show(false);
    scene.ChangeRenderWindow(this);


    if ( debugger ) debugger->Play();
}

/**
 * Test scene in an external window
 */
void SceneCanvas::OnPlayWindowBtClick( wxCommandEvent & event )
{
    scene.running = true;
    scene.editing = false;

    externalWindow->Show(true);
    externalWindow->renderCanvas->SetFramerateLimit( game.maxFPS );
    externalWindow->SetSize(GetWidth(), GetHeight());
    scene.ChangeRenderWindow(externalWindow->renderCanvas);

    scene.RenderAndStep(1);  //FIXME : Hack to make sure OpenGL Rendering is correct

    externalWindow->SetSize(GetWidth(), GetHeight());
    scene.ChangeRenderWindow(externalWindow->renderCanvas);

    if ( debugger ) debugger->Play();
}

////////////////////////////////////////////////////////////
/// Mettre la sc�ne en pause
////////////////////////////////////////////////////////////
void SceneCanvas::OnPauseBtClick( wxCommandEvent & event )
{
    scene.running = false;
    scene.editing = false;

    if ( debugger ) debugger->Pause();
}

////////////////////////////////////////////////////////////
/// Afficher le debugger de la sc�ne
////////////////////////////////////////////////////////////
void SceneCanvas::OnDebugBtClick( wxCommandEvent & event )
{
    if ( !m_mgr || !debugger ) return;

    m_mgr->GetPane(debugger.get()).Show();
    m_mgr->Update();
}

void SceneCanvas::OnKey( wxKeyEvent& evt )
{
    //Si on est en mode �diteur
    if ( scene.editing )
    {
        //ajout
        if ( evt.GetKeyCode() == WXK_INSERT )
        {
            wxCommandEvent unused;
            OnAddObjetSelected(unused);
        }
        //Suppression
        else if ( evt.GetKeyCode() == WXK_DELETE )
        {
            wxCommandEvent unused;
            OnDelObjetSelected(unused);
        }
        else if ( evt.GetKeyCode() == WXK_DOWN )
        {
            for (unsigned int i = 0;i<scene.objectsSelected.size();++i)
            {
                ObjSPtr object = scene.objectsSelected.at(i);

                int idPos = GetInitialPositionFromObject(object);
                if ( idPos != -1 )
                {
                    sceneEdited.initialObjectsPositions[idPos].y += 1;
                    object->SetY(object->GetY()+1);
                }
            }
        }
        else if ( evt.GetKeyCode() == WXK_UP )
        {
            for (unsigned int i = 0;i<scene.objectsSelected.size();++i)
            {
                ObjSPtr object = scene.objectsSelected.at(i);

                int idPos = GetInitialPositionFromObject(object);
                if ( idPos != -1 )
                {
                    sceneEdited.initialObjectsPositions[idPos].y -= 1;
                    object->SetY(object->GetY()-1);
                }
            }
        }
        else if ( evt.GetKeyCode() == WXK_RIGHT )
        {
            for (unsigned int i = 0;i<scene.objectsSelected.size();++i)
            {
                ObjSPtr object = scene.objectsSelected.at(i);

                int idPos = GetInitialPositionFromObject(object);
                if ( idPos != -1 )
                {
                    sceneEdited.initialObjectsPositions[idPos].x += 1;
                    object->SetX(object->GetX()+1);
                }
            }
        }
        else if ( evt.GetKeyCode() == WXK_LEFT )
        {
            for (unsigned int i = 0;i<scene.objectsSelected.size();++i)
            {
                ObjSPtr object = scene.objectsSelected.at(i);

                int idPos = GetInitialPositionFromObject(object);
                if ( idPos != -1 )
                {
                    sceneEdited.initialObjectsPositions[idPos].x -= 1;
                    object->SetX(object->GetX()-1);
                }
            }
        }
    }
    evt.StopPropagation();
}


void SceneCanvas::Refresh()
{
    if ( !scene.running || scene.editing )
    {
        //Reload changed images.
        for (unsigned int i = 0;i<gameEdited.imagesChanged.size();++i)
            game.imageManager->ReloadImage(gameEdited.imagesChanged[i]);

        if ( !gameEdited.imagesChanged.empty() )
        {
            gameEdited.imageManager->LoadPermanentImages();
            gameEdited.imagesChanged.clear();
            sceneEdited.wasModified = true;
        }

        if ( sceneEdited.wasModified )
            Reload();
    }
    if ( scene.running )
    {
        int retourEvent = scene.RenderAndStep(1);

        if ( retourEvent == -2 )
        {
            wxLogStatus( _( "Dans le jeu final, le jeu se terminera." ) );
        }
        else if ( retourEvent != -1 )
        {
            wxLogStatus( _( "Dans le jeu final, un changement de sc�ne s'effectuera." ) );
        }

    }
    else if ( !scene.running && !scene.editing )
        scene.RenderWithoutStep();
    else
        scene.RenderEdittimeScene();
}

void SceneCanvas::OnUpdate()
{
    Refresh();
    UpdateScrollbars();
}

void SceneCanvas::ChangesMade()
{
    //Mise � jour de l'historique d'annulation
    history.push_back(sceneEdited.initialObjectsPositions);
    redoHistory.clear();
}

////////////////////////////////////////////////////////////
/// Met � jour les barres de d�filements
////////////////////////////////////////////////////////////
void SceneCanvas::UpdateScrollbars()
{
    if ( scrollBar1 == NULL || scrollBar2 == NULL)
        return;

    //On calcule la position du thumb
    int thumbY = scene.view.GetCenter().y+scrollBar2->GetRange()/2-GetHeight()/2;
    scrollBar2->SetScrollbar(thumbY, GetHeight(), scrollBar2->GetRange(), GetHeight());

    int thumbX = scene.view.GetCenter().x+scrollBar1->GetRange()/2-GetWidth()/2;
    scrollBar1->SetScrollbar(thumbX, GetWidth(), scrollBar1->GetRange(), GetWidth());

    //On agrandit les scrollbars si besoin est
    if ( (thumbY+0) <= 0 || (thumbY+GetHeight()) >= scrollBar2->GetRange())
    {
        int ajout = GetHeight();
        scrollBar2->SetScrollbar(thumbY+ajout/2, GetHeight(), scrollBar2->GetRange()+ajout, GetHeight());
    }

    if ( (thumbX+0) <= 0 || (thumbX+GetWidth()) >= scrollBar1->GetRange())
    {
        int ajout = GetWidth();
        scrollBar1->SetScrollbar(thumbX+ajout/2, GetWidth(), scrollBar1->GetRange()+ajout, GetWidth());
    }
}

void SceneCanvas::UpdateContextMenu()
{
    //Peut on remonter les objets sur un calque plus haut ?
    int lowestLayer = GetObjectsSelectedLowestLayer();

    contextMenu.FindItem(ID_LAYERUPMENU)->Enable(false);
    if ( lowestLayer+1 < scene.initialLayers.size() )
    {
        string name = scene.initialLayers[lowestLayer+1].GetName();
        if ( name == "" ) name = _("Calque de base");
        contextMenu.FindItem(ID_LAYERUPMENU)->Enable(true);
        contextMenu.FindItem(ID_LAYERUPMENU)->SetItemLabel(string(_("Passer le(s) objet(s) sur le calque \"")) + name +"\"");
    }

    //Peut on descendre les objets sur un calque plus bas ? ( pl�onasme )
    int highestLayer = GetObjectsSelectedHighestLayer();

    contextMenu.FindItem(ID_LAYERDOWNMENU)->Enable(false);
    if ( highestLayer-1 >= 0 )
    {
        string name = scene.initialLayers[highestLayer-1].GetName();
        if ( name == "" ) name = _("Calque de base");

        contextMenu.FindItem(ID_LAYERDOWNMENU)->Enable(true);
        contextMenu.FindItem(ID_LAYERDOWNMENU)->SetItemLabel(string(_("Passer le(s) objet(s) sur le calque \"")) + name +"\"");
    }
}

////////////////////////////////////////////////////////////
/// Bouton gauche : D�placement objet
////////////////////////////////////////////////////////////
void SceneCanvas::OnLeftDown( wxMouseEvent &event )
{
    SetFocus();

    if ( !scene.editing )
        return;

    if ( hasJustRightClicked )
    {
        hasJustRightClicked = false;
        return;
    }

    ObjSPtr object = scene.FindSmallestObject();

    int mouseX = ConvertCoords(scene.input->GetMouseX(), 0).x;
    int mouseY = ConvertCoords(0, scene.input->GetMouseY()).y;

    //Suppress selection
    if ( (!scene.input->IsKeyDown(sf::Key::LShift) && !scene.input->IsKeyDown(sf::Key::RShift)) && //Check that shift is not pressed
        ( object == boost::shared_ptr<Object> () || //If no object is clicked
         find(scene.objectsSelected.begin(), scene.objectsSelected.end(), object) == scene.objectsSelected.end()) ) //Or an object which is not currently selected.
    {
        scene.objectsSelected.clear();
        scene.xObjectsSelected.clear();
        scene.yObjectsSelected.clear();

        if ( initialPositionsBrowser )
            initialPositionsBrowser->DeselectAll();
    }

    //Manage selection area
    if ( object == boost::shared_ptr<Object> () ) //If no object is clicked
    {
        //Creation
        scene.isSelecting = true;
        scene.xRectangleSelection = mouseX;
        scene.yRectangleSelection = mouseY;
        scene.xEndRectangleSelection = mouseX;
        scene.yEndRectangleSelection = mouseY;
    }

    //On ajoute l'objet surlign� dans les objets � bouger
    if ( object == boost::shared_ptr<Object> () ) return;

    //Verify if user want to resize the object
    if (    scene.objectsSelected.size() == 1 &&
            mouseX > object->GetDrawableX()+object->GetWidth()-6 &&
            mouseX < object->GetDrawableX()+object->GetWidth() &&
            mouseY > object->GetDrawableY()+object->GetHeight()/2-3 &&
            mouseY < object->GetDrawableY()+object->GetHeight()/2+3)
    {
        scene.isMovingObject = false;
        scene.isRotatingObject = false;
        scene.isResizingX = true;
    }
    else if (   scene.objectsSelected.size() == 1 &&
                mouseY > object->GetDrawableY()+object->GetHeight()-6 &&
                mouseY < object->GetDrawableY()+object->GetHeight() &&
                mouseX > object->GetDrawableX()+object->GetWidth()/2-3 &&
                mouseX < object->GetDrawableX()+object->GetWidth()/2+3 )
    {
        scene.isMovingObject = false;
        scene.isRotatingObject = false;
        scene.isResizingY = true;
    }
    else if ( scene.objectsSelected.size() == 1 &&
                mouseX > object->GetDrawableX()+object->GetWidth()/2+20*cos(object->GetAngle()/180.f*3.14159)-3 &&
                mouseX < object->GetDrawableX()+object->GetWidth()/2+20*cos(object->GetAngle()/180.f*3.14159)+3 &&
                mouseY > object->GetDrawableY()+object->GetHeight()/2+20*sin(object->GetAngle()/180.f*3.14159)-3 &&
                mouseY < object->GetDrawableY()+object->GetHeight()/2+20*sin(object->GetAngle()/180.f*3.14159)+3 )
    {
        scene.isRotatingObject = true;
        scene.isMovingObject = false;
        scene.isResizingX = false;
        scene.isResizingY = false;
    }
    else //Add object to selection
    {
        if ( find(scene.objectsSelected.begin(), scene.objectsSelected.end(), object) == scene.objectsSelected.end() )
        {
            scene.objectsSelected.push_back(object);

            //Et on renseigne sa position de d�part :
            scene.xObjectsSelected.push_back(object->GetX());
            scene.yObjectsSelected.push_back(object->GetY());

            if ( initialPositionsBrowser )
                initialPositionsBrowser->SelectInitialPosition(GetInitialPositionFromObject(object));
        }

        scene.isMovingObject = true;
        scene.isRotatingObject = false;
        scene.isResizingX = false;
        scene.isResizingY = false;
    }

    scene.oldMouseX = mouseX; //Position de d�part de la souris
    scene.oldMouseY = mouseY;
}

////////////////////////////////////////////////////////////
/// Bouton gauche relach� : Fin du d�placement
////////////////////////////////////////////////////////////
void SceneCanvas::OnLeftUp( wxMouseEvent &event )
{
    if ( !scene.editing ) return;

    //Relachement de la souris :
    //Pour les objets selectionn�s, leurs nouvelle
    //position de d�part est celle o� ils sont.
    if ( scene.isMovingObject )
    {
        for (unsigned int i = 0;i<scene.objectsSelected.size();++i)
        {
            ObjSPtr object = scene.objectsSelected.at(i);
            int IDInitialPosition = GetInitialPositionFromObject(object);
            if ( IDInitialPosition != -1)
            {
                scene.xObjectsSelected[i] = sceneEdited.initialObjectsPositions.at( IDInitialPosition ).x;
                scene.yObjectsSelected[i] = sceneEdited.initialObjectsPositions.at( IDInitialPosition ).y;

                if ( initialPositionsBrowser )
                    initialPositionsBrowser->SelectInitialPosition(IDInitialPosition);
            }
        }
    }

    //Select object thanks to the selection area
    if ( scene.isSelecting )
    {
        //Origin must be at the top left of the area
        if ( scene.xEndRectangleSelection < scene.xRectangleSelection) std::swap(scene.xEndRectangleSelection, scene.xRectangleSelection);
        if ( scene.yEndRectangleSelection < scene.yRectangleSelection) std::swap(scene.yEndRectangleSelection, scene.yRectangleSelection);

        ObjList allObjects = scene.objectsInstances.GetAllObjects();

        for (unsigned int id = 0;id < allObjects.size();++id)
        {
            //Find and add to selection all objects of the selection area
            ObjSPtr object = allObjects[id];
            if ( object->GetX() >= scene.xRectangleSelection &&
                 object->GetX()+object->GetWidth() <= scene.xEndRectangleSelection &&
                 object->GetY() >= scene.yRectangleSelection &&
                 object->GetY()+object->GetHeight() <= scene.yEndRectangleSelection )
             {
                int IDInitialPosition = GetInitialPositionFromObject(object);
                if ( IDInitialPosition != -1)
                {
                    if ( find(scene.objectsSelected.begin(), scene.objectsSelected.end(), object) == scene.objectsSelected.end() )
                    {
                        scene.objectsSelected.push_back(object);

                        //Et on renseigne sa position de d�part :
                        scene.xObjectsSelected.push_back(object->GetX());
                        scene.yObjectsSelected.push_back(object->GetY());

                        if ( initialPositionsBrowser )
                            initialPositionsBrowser->SelectInitialPosition(IDInitialPosition);
                    }
                }
             }
        }
    }

    scene.isResizingX = false;
    scene.isResizingY = false;
    scene.isMovingObject = false;
    scene.isRotatingObject = false;
    scene.isSelecting = false;

    ChangesMade();
}

////////////////////////////////////////////////////////////
/// A chaque d�placement de la souris :
///
/// -affichage position
/// -bouger la vue si on suit la souris
////////////////////////////////////////////////////////////
void SceneCanvas::OnMotion( wxMouseEvent &event )
{
    //Mille mercis Laurent.
    int mouseXInScene = 0;
    int mouseYInScene = 0;

    wxString Xstr;
    wxString Ystr;

    if ( !scene.editing )
    {
        mouseXInScene = static_cast<int>(sf::RenderWindow::ConvertCoords(scene.input->GetMouseX(), 0, scene.GetLayer("").GetCamera(0).GetSFMLView()).x);
        mouseYInScene = static_cast<int>(sf::RenderWindow::ConvertCoords(0, scene.input->GetMouseY(), scene.GetLayer("").GetCamera(0).GetSFMLView()).y);
        Xstr =ToString( mouseXInScene );
        Ystr =ToString( mouseYInScene );

        wxLogStatus( wxString( _( "Position " ) ) + Xstr + wxString( _( ";" ) ) + Ystr + wxString( _( ". ( Calque de base, Cam�ra 0 )" ) ) );
    }
    else
    {
        mouseXInScene = static_cast<int>(sf::RenderWindow::ConvertCoords(scene.input->GetMouseX(), 0).x);
        mouseYInScene = static_cast<int>(sf::RenderWindow::ConvertCoords(0, scene.input->GetMouseY()).y);

        Xstr =ToString( mouseXInScene );
        Ystr =ToString( mouseYInScene );

        wxLogStatus( wxString( _( "Position " ) ) + Xstr + wxString( _( ";" ) ) + Ystr + wxString( _( ". SHIFT pour s�lection multiple, clic droit pour plus d'options." ) ) );
    }

    //Le reste concerne juste le mode �dition
    if ( scene.running )
        return;

    //D�placement avec la souris
    if ( scene.isMoving )
        scene.view.Move( scene.deplacementOX - mouseXInScene, scene.deplacementOY - mouseYInScene );

    if ( scene.isResizingX )
    {
        for (unsigned int i = 0;i<scene.objectsSelected.size();++i)
        {
            ObjSPtr object = scene.objectsSelected.at(i);
            object->SetWidth(mouseXInScene-scene.xObjectsSelected.at(i));

            int idPos = GetInitialPositionFromObject(object);
            if ( idPos != -1 )
            {
                sceneEdited.initialObjectsPositions.at( idPos ).personalizedSize = true;
                sceneEdited.initialObjectsPositions.at( idPos ).width = object->GetWidth();
                sceneEdited.initialObjectsPositions.at( idPos ).height = object->GetHeight();
            }
        }
    }
    if ( scene.isResizingY )
    {
        for (unsigned int i = 0;i<scene.objectsSelected.size();++i)
        {
            ObjSPtr object = scene.objectsSelected.at(i);
            object->SetHeight(mouseYInScene-scene.yObjectsSelected.at(i));

            int idPos = GetInitialPositionFromObject(object);
            if ( idPos != -1 )
            {
                sceneEdited.initialObjectsPositions.at( idPos ).personalizedSize = true;
                sceneEdited.initialObjectsPositions.at( idPos ).height = object->GetHeight();
                sceneEdited.initialObjectsPositions.at( idPos ).width = object->GetWidth();
            }
        }
    }
    if ( scene.isRotatingObject )
    {
        for (unsigned int i = 0;i<scene.objectsSelected.size();++i)
        {
            ObjSPtr object = scene.objectsSelected.at(i);
            float x = mouseXInScene-(object->GetDrawableX()+object->GetWidth()/2);
            float y = mouseYInScene-(object->GetDrawableY()+object->GetHeight()/2);
            float newAngle = atan2(y,x)*180/3.14159;

            object->SetAngle(newAngle);

            int idPos = GetInitialPositionFromObject(object);
            if ( idPos != -1 )
            {
                sceneEdited.initialObjectsPositions.at( idPos ).angle = newAngle;
            }
        }
    }
    //D�placement de la position initiale d'un objet
    if ( scene.isMovingObject )
    {
        for (unsigned int i = 0;i<scene.objectsSelected.size();++i)
        {
            ObjSPtr object = scene.objectsSelected.at(i);

            int idPos = GetInitialPositionFromObject(object);
            if ( idPos != -1 )
            {
                //D�placement effectu� par la souris
                int deltaX = mouseXInScene - scene.oldMouseX;
                int deltaY = mouseYInScene - scene.oldMouseY;

                //Anciennes et nouvelles coordonn�es
                int oldX = scene.xObjectsSelected[i];
                int oldY = scene.yObjectsSelected[i];
                int newX = oldX + deltaX;
                int newY = oldY + deltaY;

                if ( scene.grid && scene.snap )
                {
                    newX = static_cast<int>(newX/scene.gridWidth)*scene.gridWidth;
                    newY = static_cast<int>(newY/scene.gridHeight)*scene.gridHeight;
                }

                //Modification de l'emplacement initial
                sceneEdited.initialObjectsPositions.at( idPos ).x = newX;
                sceneEdited.initialObjectsPositions.at( idPos ).y = newY;

                //On bouge aussi l'objet actuellement affich�
                object->SetX( newX );
                object->SetY( newY );
            }
        }
    }
    if ( scene.isSelecting )
    {
        scene.xEndRectangleSelection = mouseXInScene;
        scene.yEndRectangleSelection = mouseYInScene;
    }

}

////////////////////////////////////////////////////////////
/// Double clic : insertion objet
////////////////////////////////////////////////////////////
void SceneCanvas::OnLeftDClick( wxMouseEvent &event )
{
    AddObjetSelected(ConvertCoords(scene.input->GetMouseX(), 0).x, ConvertCoords(0, scene.input->GetMouseY()).y);
}

////////////////////////////////////////////////////////////
/// Insertion d'un objet
////////////////////////////////////////////////////////////
void SceneCanvas::OnAddObjetSelected( wxCommandEvent & event )
{
    AddObjetSelected(ConvertCoords(scene.input->GetMouseX(), 0).x, ConvertCoords(0, scene.input->GetMouseY()).y);
}

void SceneCanvas::AddObjetSelected(float x, float y)
{
    //Seulement en mode �diteur
    if ( !scene.editing )
        return;

    scene.isMovingObject = false;

    if ( scene.objectToAdd == "" ) { wxLogMessage( _( "Vous n'avez selectionn� aucun objet � ajouter.\nS�lectionnez en un avec le bouton \"Choisir un objet � ajouter\" dans la barre d'outils." ) ); return;}

    int IDsceneObject = Picker::PickOneObject( &sceneEdited.initialObjects, scene.objectToAdd );
    int IDglobalObject = Picker::PickOneObject( &gameEdited.globalObjects, scene.objectToAdd );

    ObjSPtr newObject = boost::shared_ptr<Object> ();

    if ( IDsceneObject != -1 ) //We check first scene's objects' list.
        newObject = sceneEdited.initialObjects[IDsceneObject]->Clone();
    else if ( IDglobalObject != -1 ) //Then the global object list
        newObject = gameEdited.globalObjects[IDglobalObject]->Clone();

    if ( newObject == boost::shared_ptr<Object> () )
    {
        wxLogMessage(_("L'objet � ajouter n'existe pas ou plus dans la liste des objets."));
        return;
    }

    //Initial position creation
    InitialPosition pos;
    pos.objectName = scene.objectToAdd; //A choisir avec un dialog appropri� ou par drag'n'drop
    if ( scene.grid && scene.snap )
    {
        pos.x = static_cast<int>(x/scene.gridWidth)*scene.gridWidth;
        pos.y = static_cast<int>(y/scene.gridHeight)*scene.gridHeight;
    }
    else
    {
        pos.x = x;
        pos.y = y;
    }

    pos.zOrder = 0;
    pos.layer = scene.addOnLayer;
    sceneEdited.initialObjectsPositions.push_back( pos );

    //Edittime scene object creation
    newObject->errors = &scene.errors;
    newObject->SetX( pos.x );
    newObject->SetY( pos.y );
    newObject->SetZOrder( pos.zOrder );
    newObject->SetLayer( pos.layer );

    newObject->InitializeFromInitialPosition(pos);
    newObject->LoadRuntimeResources( *game.imageManager );

    scene.objectsInstances.AddObject(newObject);

    newObject->LoadResources(*game.imageManager); //Global objects images are curiously not displayed if we don't reload resources..

    ChangesMade();
}

////////////////////////////////////////////////////////////
/// Clic droit : edition propri�t�s objet
////////////////////////////////////////////////////////////
void SceneCanvas::OnRightUp( wxMouseEvent &event )
{
    if ( !scene.editing )
        return;

    ObjSPtr object = scene.FindSmallestObject();

    //Suppress selection if
    if ( object == boost::shared_ptr<Object> () || /*Not clicked on an object*/
        (( !scene.input->IsKeyDown(sf::Key::LShift) && !scene.input->IsKeyDown(sf::Key::RShift) ) && /*Clicked without using shift*/
         find(scene.objectsSelected.begin(), scene.objectsSelected.end(), object) == scene.objectsSelected.end() ))
    {
        scene.objectsSelected.clear();
        scene.xObjectsSelected.clear();
        scene.yObjectsSelected.clear();

        if ( initialPositionsBrowser )
            initialPositionsBrowser->DeselectAll();
    }

    if ( object == boost::shared_ptr<Object> () ) //Popup "no object" context menu
    {
        PopupMenu(&noObjectContextMenu);
        return;
    }

    //Add the object to selection
    if ( find(scene.objectsSelected.begin(), scene.objectsSelected.end(), object) == scene.objectsSelected.end() )
    {
        scene.objectsSelected.push_back(object);

        //Must also register its position
        scene.xObjectsSelected.push_back(object->GetX());
        scene.yObjectsSelected.push_back(object->GetY());

        if ( initialPositionsBrowser )
            initialPositionsBrowser->SelectInitialPosition(GetInitialPositionFromObject(object));
    }

    OnUpdate(); //So as to display selection rectangle for the newly selected object
    UpdateContextMenu();
    PopupMenu(&contextMenu);

    hasJustRightClicked = true;
}

////////////////////////////////////////////////////////////
/// D�placement de(s) objet(s) selectionn�(s) sur le calque sup�rieur
////////////////////////////////////////////////////////////
void SceneCanvas::OnLayerUpSelected(wxCommandEvent & event)
{
    int lowestLayer = GetObjectsSelectedLowestLayer();
    if ( lowestLayer+1 < 0 || static_cast<unsigned>(lowestLayer+1) >= scene.initialLayers.size() )
        return;

    string layerName = scene.initialLayers.at(lowestLayer+1).GetName();

    for (unsigned int i =0;i<scene.objectsSelected.size();++i)
    {
        //R�cup�rons la position initiale
        int posId = GetInitialPositionFromObject(scene.objectsSelected[i]);
        if ( posId != -1 )
        {
            sceneEdited.initialObjectsPositions.at(posId).layer = layerName;
            scene.objectsSelected[i]->SetLayer(layerName);
        }
    }

    ChangesMade();
}

void SceneCanvas::OnCopySelected(wxCommandEvent & event)
{
    vector < InitialPosition > copiedPositions;

    for (unsigned int i =0;i<scene.objectsSelected.size();++i)
    {
        int posId = GetInitialPositionFromObject(scene.objectsSelected[i]);
        if ( posId != -1 )
        {
            copiedPositions.push_back(sceneEdited.initialObjectsPositions.at(posId));
            copiedPositions.back().x -= ConvertCoords(scene.input->GetMouseX(), 0).x;
            copiedPositions.back().y -= ConvertCoords(0, scene.input->GetMouseY()).y;
        }
    }

    Clipboard::getInstance()->SetPositionsSelection(copiedPositions);
}

void SceneCanvas::OnCutSelected(wxCommandEvent & event)
{
    vector < InitialPosition > copiedPositions;

    for (unsigned int i =0;i<scene.objectsSelected.size();++i)
    {
        int posId = GetInitialPositionFromObject(scene.objectsSelected[i]);
        if ( posId != -1 )
        {
            //Copy position
            copiedPositions.push_back(sceneEdited.initialObjectsPositions.at(posId));
            copiedPositions.back().x -= ConvertCoords(scene.input->GetMouseX(), 0).x;
            copiedPositions.back().y -= ConvertCoords(0, scene.input->GetMouseY()).y;

            //Remove objects
            sceneEdited.initialObjectsPositions.erase(sceneEdited.initialObjectsPositions.begin() + posId);
            scene.objectsInstances.RemoveObject(scene.objectsSelected[i]);
        }
    }

    scene.objectsSelected.clear();
    scene.xObjectsSelected.clear();
    scene.yObjectsSelected.clear();

    if ( initialPositionsBrowser )
        initialPositionsBrowser->DeselectAll();

    Clipboard::getInstance()->SetPositionsSelection(copiedPositions);
}

void SceneCanvas::OnPasteSelected(wxCommandEvent & event)
{
    if ( !Clipboard::getInstance()->HasPositionsSelection() ) return;

    vector < InitialPosition > pastedPositions = Clipboard::getInstance()->GetPositionsSelection();

    for (unsigned int i =0;i<pastedPositions.size();++i)
    {
        sceneEdited.initialObjectsPositions.push_back(pastedPositions[i]);
        sceneEdited.initialObjectsPositions.back().x += ConvertCoords(scene.input->GetMouseX(), 0).x;
        sceneEdited.initialObjectsPositions.back().y += ConvertCoords(0, scene.input->GetMouseY()).y;
    }

    Reload();
}

////////////////////////////////////////////////////////////
/// D�placement de(s) objet(s) selectionn�(s) sur le calque inf�rieur
////////////////////////////////////////////////////////////
void SceneCanvas::OnLayerDownSelected(wxCommandEvent & event)
{
    int highestLayer = GetObjectsSelectedLowestLayer();
    if ( highestLayer-1 < 0 || static_cast<unsigned>(highestLayer-1) >= scene.initialLayers.size() )
        return;

    string layerName = scene.initialLayers.at(highestLayer-1).GetName();

    for (unsigned int i =0;i<scene.objectsSelected.size();++i)
    {
        //R�cup�rons la position initiale
        int posId = GetInitialPositionFromObject(scene.objectsSelected[i]);
        if ( posId != -1 )
        {
            sceneEdited.initialObjectsPositions.at(posId).layer = layerName;
            scene.objectsSelected[i]->SetLayer(layerName);
        }
    }

    ChangesMade();
}

////////////////////////////////////////////////////////////
/// Editer les valeurs initiales d'un objet sur la sc�ne
////////////////////////////////////////////////////////////
void SceneCanvas::OnPropObjSelected(wxCommandEvent & event)
{
    if ( !scene.editing )
        return;

    //Cherche l'objet sous la souris
    ObjSPtr smallestObject = scene.FindSmallestObject();
    if ( smallestObject == boost::shared_ptr<Object> ()) return;

    int idPos = GetInitialPositionFromObject(smallestObject);
    if ( idPos == -1 ) return;

    bool hadAPersonalizedSize = sceneEdited.initialObjectsPositions.at( idPos ).personalizedSize;

    //Affichage des propri�t�s de l'objet sous la souris
    EditOptionsPosition DialogPosition( this, gameEdited, scene, sceneEdited.initialObjectsPositions.at( idPos ) );
    if ( DialogPosition.ShowModal() == 1 )
    {
        sceneEdited.initialObjectsPositions.at( idPos ) = DialogPosition.position;

        smallestObject->SetX( sceneEdited.initialObjectsPositions.at( idPos ).x );
        smallestObject->SetY( sceneEdited.initialObjectsPositions.at( idPos ).y );
        smallestObject->SetZOrder( sceneEdited.initialObjectsPositions.at( idPos ).zOrder );
        smallestObject->SetLayer( sceneEdited.initialObjectsPositions.at( idPos ).layer );

        smallestObject->InitializeFromInitialPosition(sceneEdited.initialObjectsPositions.at( idPos ));

        if ( sceneEdited.initialObjectsPositions.at( idPos ).personalizedSize )
        {
            smallestObject->SetWidth( sceneEdited.initialObjectsPositions.at( idPos ).width );
            smallestObject->SetHeight( sceneEdited.initialObjectsPositions.at( idPos ).height );
        }
        else if ( hadAPersonalizedSize ) //For now, we reload the scene so as the object get back its initial size
        {
            Reload();
        }
        ChangesMade();
    }

    return;
}

////////////////////////////////////////////////////////////
/// Double clic droit : propri�t�s direct de l'objet
////////////////////////////////////////////////////////////
void SceneCanvas::OnRightDClick( wxMouseEvent &event )
{
    wxCommandEvent unusedEvent;
    OnPropObjSelected(unusedEvent);
}

////////////////////////////////////////////////////////////
/// Suppression de(s) objet(s) selectionn�(s)
////////////////////////////////////////////////////////////
void SceneCanvas::OnDelObjetSelected(wxCommandEvent & event)
{
    if ( !scene.editing )
        return;

    for (unsigned int i = 0;i<scene.objectsSelected.size();++i)
    {
        ObjSPtr object = scene.objectsSelected.at(i);

        int idPos = GetInitialPositionFromObject(object);
        if ( idPos != -1 )
        {
            sceneEdited.initialObjectsPositions.erase(sceneEdited.initialObjectsPositions.begin() + idPos);
            scene.objectsInstances.RemoveObject(object);
        }
    }

    scene.objectsSelected.clear();
    scene.xObjectsSelected.clear();
    scene.yObjectsSelected.clear();

    if ( initialPositionsBrowser )
        initialPositionsBrowser->Refresh();

    ChangesMade();
}

////////////////////////////////////////////////////////////
/// Clic molette : Des/activer d�placement � la souris
////////////////////////////////////////////////////////////
void SceneCanvas::OnMiddleDown( wxMouseEvent &event )
{
    if ( !scene.editing ) return;

    if ( !scene.isMoving )
    {
        scene.isMoving = true;
        scene.deplacementOX = ConvertCoords(scene.input->GetMouseX(), 0).x;
        scene.deplacementOY = ConvertCoords(0, scene.input->GetMouseY()).y;
        SetCursor( wxCursor( wxCURSOR_SIZING ) );
        return;
    }
    else
    {
        scene.isMoving = false;
        SetCursor( wxNullCursor );
    }
}

void SceneCanvas::OnMiddleUp( wxMouseEvent &event )
{
}


void SceneCanvas::Reload()
{
    game = gameEdited;
    game.imageManager = gameEdited.imageManager; //Use same image manager.

    scene.StopMusic();
    scene.LoadFromScene( sceneEdited );

    sceneEdited.wasModified = false;

    UpdateScrollbars();
}

////////////////////////////////////////////////////////////
/// Zoom / dezoom � la molette
/// Il faut prendre garde � garder les proportions de la fen�tre
////////////////////////////////////////////////////////////
void SceneCanvas::OnMouseWheel( wxMouseEvent &event )
{
    if (scene.running)
        return;

    //La rotation de la molette
    float rotation = event.GetWheelRotation()*3;
    scene.zoom += ( rotation / 25 );

    //Le rapport entre la largeur et la hauteur
    float qwoh = scene.view.GetSize().x / scene.view.GetSize().y;

    //La nouvelle hauteur
    float newheight = scene.view.GetSize().y + ( rotation / 25 );

    scene.view.SetSize( newheight*qwoh, newheight );
}

int SceneCanvas::GetObjectsSelectedHighestLayer()
{
    int highestLayer = 0;
    for (unsigned int i =0;i<scene.objectsSelected.size();++i)
    {
        //R�cup�rons la position initiale
        int posId = GetInitialPositionFromObject(scene.objectsSelected[i]);
        if ( posId != -1 )
        {
            int layerObjId = 0;
            //On cherche le num�ro du calque de l'objet
            for (unsigned int layerId = 0;layerId<scene.initialLayers.size();++layerId)
            {
                if ( scene.initialLayers[layerId].GetName() == sceneEdited.initialObjectsPositions.at(posId).layer )
                   layerObjId = layerId;
            }

            if ( layerObjId > highestLayer )
                highestLayer = layerObjId;
        }
    }

    return highestLayer;
}

int SceneCanvas::GetObjectsSelectedLowestLayer()
{
    int lowestLayer = scene.initialLayers.size()-1;
    for (unsigned int i =0;i<scene.objectsSelected.size();++i)
    {
        //R�cup�rons la position initiale
        int posId = GetInitialPositionFromObject(scene.objectsSelected[i]);
        if ( posId != -1 )
        {
            int layerObjId = 0;
            //On cherche le num�ro du calque de l'objet
            for (unsigned int layerId = 0;layerId<scene.initialLayers.size();++layerId)
            {
                if ( scene.initialLayers[layerId].GetName() == sceneEdited.initialObjectsPositions.at(posId).layer )
                   layerObjId = layerId;
            }

            if ( layerObjId < lowestLayer )
                lowestLayer = layerObjId;
        }
    }

    return lowestLayer;
}

////////////////////////////////////////////////////////////
/// Renvoi l'ID d'une position initiale � partir d'un objet sur la sc�ne
////////////////////////////////////////////////////////////
int SceneCanvas::GetInitialPositionFromObject(ObjSPtr object)
{
    if ( object == boost::shared_ptr<Object> ()) return -1;

    for (unsigned int j = 0;j < sceneEdited.initialObjectsPositions.size();++j)
    {
        if ( sceneEdited.initialObjectsPositions.at( j ).objectName == object->GetName() &&
                sceneEdited.initialObjectsPositions.at( j ).x == object->GetX() &&
                sceneEdited.initialObjectsPositions.at( j ).y == object->GetY() )
        {
            return j;
        }
    }

    return -1;
}
