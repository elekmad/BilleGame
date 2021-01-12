/*
 * BallGame.h
 *
 *  Created on: 15 nov. 2020
 *      Author: damien
 */

#ifndef BALLGAME_H_
#define BALLGAME_H_

#include <Ogre.h>
#include <OgreRTShaderSystem.h>
#include <OgreMath.h>
#include <iostream>
#include "BaseApplication.h"


#include <Newton.h>
#include <toolbox_stdafx.h>
#include <dHighResolutionTimer.h>
#include <DebugDisplay.h>

#include <dList.h>
#include <dMatrix.h>

//because of Xlib defines (True False Bool None) which must be undef
#undef True
#undef False
#undef None
#undef Bool
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <CEGUI/CEGUI.h>
#include <CEGUI/RendererModules/Ogre/Renderer.h>

#define LEVELS_FOLDER "./Levels/"
#define LEVELS_EXTENSION "json"
#define STATES_EXTENSION "state"

using namespace Ogre;
using namespace OgreBites;

#define MAX_PHYSICS_FPS				240.0f
#define MAX_PHYSICS_SUB_STEPS		50
//#define PROJECTILE_INITIAL_SPEED	20.0f

#ifdef OGRE_STATIC_LIB
#  define OGRE_STATIC_GL
#  if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#    define OGRE_STATIC_Direct3D9
// dx10 will only work on vista, so be careful about statically linking
#    if OGRE_USE_D3D10
#      define OGRE_STATIC_Direct3D10
#    endif
#  endif
#  define OGRE_STATIC_BSPSceneManager
#  define OGRE_STATIC_ParticleFX
#  define OGRE_STATIC_CgProgramManager
#  ifdef OGRE_USE_PCZ
#    define OGRE_STATIC_PCZSceneManager
#    define OGRE_STATIC_OctreeZone
#  else
#    define OGRE_STATIC_OctreeSceneManager
#  endif
#  include "OgreStaticPluginLoader.h"
#endif

class BallGame;
class GroupEntity;

enum BallGameEntityType
{
	Case,
	Ball
};

class BallGameEntity
{
	public :

	BallGameEntity(const dMatrix& matrix);
	BallGameEntity();
	~BallGameEntity(){}
	void Finalize(void);
    static void TransformCallback(const NewtonBody* body, const dFloat* matrix, int threadIndex);
    void ExportToJson(rapidjson::Value &v, rapidjson::Document::AllocatorType& allocator);
    void ImportFromJson(rapidjson::Value &v, BallGame *Game, Node *parent, String &nodeNamePrefix);
    void setOgreNode(SceneNode *node);
    void setNewtonBody(NewtonBody *body);
    const NewtonBody *getNewtonBody(void) const { return Body; }
    const SceneNode *getOgreEntity(void) const { return OgreEntity; }
    dMatrix *PrepareNewtonBody(dVector &NewtonBodyLocation, dVector &NewtonBodySize);
    void DisplaySelectedBox(bool display);
    const Ogre::Vector3 &getInitialPosition(void) const { return InitialPos; }
    const Ogre::Vector3 &getRelativePosition(void) const { return OgreEntity->getPosition(); }
    const Ogre::Vector3 &getAbsolutePosition(void) const { return OgreEntity->_getDerivedPosition(); }
    void setInitialPosition(const Ogre::Vector3 &NewPosition) { InitialPos = NewPosition; }
    void setRelativePosition(const Ogre::Vector3 &NewPosition) { OgreEntity->setPosition(NewPosition); }
    void setAbsolutePosition(const Ogre::Vector3 &NewPosition) { OgreEntity->_setDerivedPosition(NewPosition); }
    const Ogre::Quaternion &getInitialOrientation(void) const { return InitialOrientation; }
    const Ogre::Quaternion &getRelativeOrientation(void) const { return OgreEntity->getOrientation(); }
    const Ogre::Quaternion &getAbsoluteOrientation(void) const { return OgreEntity->_getDerivedOrientation(); }
    void setInitialOrientation(const Ogre::Quaternion &NewOrient) { InitialOrientation = NewOrient; }
    void setRelativeOrientation(const Ogre::Quaternion &NewOrient) { OgreEntity->setOrientation(NewOrient); }
    void setAbsoluteOrientation(const Ogre::Quaternion &NewOrient) { OgreEntity->_setDerivedOrientation(NewOrient); }
    const Ogre::Vector3 &getInitialScale(void) const { return InitialScale; }
    const Ogre::Vector3 &getRelativeScale(void) const { return OgreEntity->getScale(); }
    const Ogre::Vector3 &getAbsoluteScale(void) const { return OgreEntity->_getDerivedScale(); }
    void setInitialScale(const Ogre::Vector3 &NewScale) { InitialScale = NewScale; }
    void setRelativeScale(const Ogre::Vector3 &NewScale) { OgreEntity->setScale(NewScale); }
    const Ogre::String &getName(void) const { return OgreEntity->getName(); }
    void Move(float x, float y, float z);
    void Move(Vector3 &);
    void Rotate(float x, float y, float z);
    void Scale(float x, float y, float z);
    void Scale(Vector3 &);
    void getVelocity(dFloat *Velocity) { NewtonBodyGetVelocity(Body, Velocity); }
    enum BallGameEntityType getType(void) { return type; }
    GroupEntity *getGroup(void) { return Group; }

	protected :

    enum BallGameEntityType type;
	//mutable dMatrix m_matrix;			// interpolated matrix
	dVector m_curPosition;				// position one physics simulation step in the future
	dVector m_nextPosition;             // position at the current physics simulation step
	dQuaternion m_curRotation;          // rotation one physics simulation step in the future
	dQuaternion m_nextRotation;         // rotation at the current physics simulation step
	SceneNode *OgreEntity;
	NewtonBody *Body;
	GroupEntity *Group;
	//We need to have initial pos, scale and orientation appart from ogre's one because we can edit level during physic move, so export level with ogre's one can be impossible !
	Ogre::Vector3 InitialPos;
	Ogre::Vector3 InitialScale;
	Ogre::Quaternion InitialOrientation;

    void SetMatrixUsafe(const dQuaternion& rotation, const dVector& position);

	friend class GroupEntity;
};

class BallEntity : public BallGameEntity
{
	public :

	BallEntity(const dMatrix& matrix);
	BallEntity();
	void CreateFromJson(rapidjson::Value &v, BallGame *Game, NewtonWorld *m_world, Node *parent, String &nodeNamePrefix);
	void AddForceVector(dVector *force);
    void ExportToJson(rapidjson::Value &v, rapidjson::Document::AllocatorType& allocator);
    void ImportFromJson(rapidjson::Value &v, BallGame *Game, Node *parent, String &nodeNamePrefix);
	dVector *GetForceVector();
	void CleanupForces(void);
    void CreateNewtonBody(NewtonWorld *m_world);
    dFloat getMass(void);
    void setMass(dFloat newMass);
    float getInitialMass(void) { return InitialMass; }
    void setInitialMass(float newInitialMass) { InitialMass = newInitialMass; }

	protected :

	dList<dVector*> Forces;
	float InitialMass;
};

class CaseEntity : public BallGameEntity
{
	public :

	enum CaseType
	{
		typeBox = 0,
		typeRamp = 1
	};
	enum CaseType type;
	CaseEntity(const dMatrix& matrix, enum CaseType _type = typeBox);
	CaseEntity(enum CaseType _type = typeBox);
	~CaseEntity(){ BallsUnderCollide.clear(); }
	void CreateFromJson(rapidjson::Value &v, BallGame *Game, NewtonWorld *m_world, Node *parent, String &nodeNamePrefix);
//	void AddBallColliding(NewtonBody *ball);
//	bool CheckIfAlreadyColliding(NewtonBody *ball);
	void SetForceToApply(float force, dVector *direction);
	void ApplyForceOnBall(BallEntity *ball);
    void ExportToJson(rapidjson::Value &v, rapidjson::Document::AllocatorType& allocator);
    void ImportFromJson(rapidjson::Value &v, BallGame *Game, Node *parent, String &nodeNamePrefix);
    void CreateNewtonBody(NewtonWorld *m_world);
    float getForce(void) { return force_to_apply; }
    const dVector *getForceDirection(void) { return force_direction; }
    void AddBallColliding(BallEntity *ball);
	void ApplyForceOnCollidingBalls(void);
	Ogre::SceneNode *CreateForceArrows(Ogre::SceneManager *Scene);

	protected :

    bool CheckIfAlreadyColliding(BallEntity *ball);

	std::list<BallEntity*> BallsUnderCollide;

	float force_to_apply;
	dVector *force_direction;
};

class GroupEntity
{
	public :

	GroupEntity(String &name, Ogre::SceneManager* mSceneMgr);
	GroupEntity(){ OgreEntity = NULL; computed = false; equilibrated = false; };
	~GroupEntity(){};
	void Finalize(void);
    void ExportToJson(rapidjson::Value &v, rapidjson::Document::AllocatorType& allocator);
    void ImportFromJson(rapidjson::Value &v, Node *parent, String &nodeNamePrefix);
	void AddChild(BallGameEntity* child);
	bool DelChild(BallGameEntity* child);
	void ComputeChilds(void);
	void ComputeAndEquilibrateChilds(void);
	void FillListWithChilds(std::list<BallGameEntity*> &list);
    void Move(float x, float y, float z);
    void Rotate(float x, float y, float z);
    void Scale(float x, float y, float z);
    const Ogre::String &getName(void) const { return OgreEntity->getName(); }

	private :

	SceneNode *OgreEntity;
	std::list<BallGameEntity*> childs;
	bool computed;
	bool equilibrated;
};

class BallGame : public BaseApplication
{
    public :

    BallGame();
    ~BallGame();
    GroupEntity *findGroup(const char * const name);
    void AddGroup(GroupEntity *Entity);

    private :

    class EntityType
	{
	public :

    	EntityType(){ InitialMass = 0.0; Type= Case; }
    	~EntityType(){}
		String Name;
		enum BallGameEntityType Type;
		String MeshName;
		Vector3 InitialPos;
		Vector3 InitialScale;
		Quaternion InitialOrientation;
		float InitialMass;
	};

    std::list<class EntityType*> EntityTypes;

    void LoadBallGameEntityTypes(void);

    void Append(BallGameEntity *entity);
    void _StartPhysic(void);
    void _StopPhysic(void);
    void SwitchEditMode(void);
    void DeleteCase(CaseEntity *Entity, std::list<CaseEntity*>::iterator *iter = NULL);
    void RemoveCase(CaseEntity *Entity, std::list<CaseEntity*>::iterator *iter = NULL);
    void DeleteBall(BallEntity *Entity, std::list<BallEntity*>::iterator *iter = NULL);
    void RemoveBall(BallEntity *Entity, std::list<BallEntity*>::iterator *iter = NULL);
    void DeleteGroup(GroupEntity *Entity, std::list<GroupEntity*>::iterator *iter = NULL);
    void RemoveGroup(GroupEntity *Entity, std::list<GroupEntity*>::iterator *iter = NULL);
    void LoadStatesList(void);
    void EmptyStatesList(void);
    void EmptyLevelsList(void);
    void EmptyLevel(void);//Clean all BallGame, Newton and Ogre entities to start with new level.
    void ChangeLevel(void);
    void ImportLevelFromJson(Node *parent, String &nodeNamePrefix, bool isForImport = false);

    String Level;
    String LevelFilename;
    void SetLevel(String &level_name, String &levelFilename);

    //Place New Element
    enum PlacementModes
	{
    	PlaceMove,
		PlaceRotate,
		PlaceScale,
    	EditMove,
		EditRotate,
		EditScale,
		Delete
	}PlacementMode;

	EntityType *ToBePlacedEntityType;
	SceneNode *ogreThumbnailNode;
    BallGameEntity *ToBePlacedEntity;
    BallGameEntity *LastPlacedEntity;
    BallGameEntity *ToBeDeletedEntity;
    void PlaceNewElement(void);
    void PlaceUnderEditElement(void);
    void PlaceElement(BallGameEntity *ToBePlacedEntity);
    void PrepareNewElement(void);
    inline void UnprepareNewElement(void);
    void DeleteElement(void);
    inline void PrepareDeleteElement(BallGameEntity *Entity);
    inline void UnprepareDeleteElement(void);

    //Edit Entities
    bool MultiSelectionMode;
    void MoveEntities(float x, float y, float z);
    void RotateEntities(float x, float y, float z);
    void ScaleEntities(float x, float y, float z);
    void MultiSelectionSetEmpty(void);
    bool ManageMultiSelectionSet(BallGameEntity *entity);
    std::list<class BallGameEntity*> UnderEditEntites;
    //Edit Ball
    void EditBall(BallEntity *Entity);
    BallEntity *UnderEditBall;
    dFloat UnderEditBallMass;

    //Edit Case
    bool MouseOverButton;
    void EditCase(CaseEntity *Entity);
	CaseEntity *UnderEditCase;
	bool CaseHasForce;
	float UnderEditCaseForce;
	bool force_directed;
	dVector force_direction;
	Ogre::SceneNode *ForcesArrows;
	void UpdateForceArrows(void);
    void UpdateEditButtons(void);

    enum RunningMode
    {
		Running,
		Editing
    }mode;
    /////////////////////  NEWTON ///////////////////
    public :

    NewtonWorld* m_world;
    static void PostUpdateCallback(const NewtonWorld* const world, dFloat timestep);
    static void OnContactCollision (const NewtonJoint* contactJoint, dFloat timestep, int threadIndex);
    void UpdatePhysics(dFloat timestep);

    private :

    void SetupNewton(void);
    NewtonWorld* GetNewton(void);
    static void BodySerialization (NewtonBody* const body, void* const bodyUserData, NewtonSerializeCallback serializeCallback, void* const serializeHandle);
    static void BodyDeserialization (NewtonBody* const body, void* const bodyUserData, NewtonDeserializeCallback deserializecallback, void* const serializeHandle);
    void SerializedPhysicScene(const String* const name);
    void DeserializedPhysicScene(const String* const name);
    BallGameEntity *GetEntity(char *name);
    bool CheckIfAlreadyColliding(CaseEntity *ToCheck);
    void AddCaseColliding(CaseEntity *ToAdd);

    bool m_suspendPhysicsUpdate;
    unsigned64 m_microsecunds;
    int m_physicsFramesCount;
    dFloat m_mainThreadPhysicsTime;
    dFloat m_mainThreadPhysicsTimeAcc;
    bool m_asynchronousPhysicsUpdate;

    unsigned long nb_entities;
    std::list<CaseEntity*> Cases;
    std::list<BallEntity*> Balls;
    std::list<GroupEntity*> Groups;
	std::list<CaseEntity*> CasesUnderCollide;

	String ImportLevelFilename;
	String ImportLevelName;
    std::list<CaseEntity*> ImportLevelCases;
    std::list<BallEntity*> ImportLevelBalls;
    std::list<GroupEntity*> ImportLevelGroups;
    inline void ActivateLevelImportInterface(void);
    inline void UnactivateLevelImportInterface(void);

    void CheckforCollides(void);
    void AddCase(CaseEntity *Entity);
    void AddBall(BallEntity *Entity);

    /////////////////////////////////////////////////


    ////////////////  Ogre ////////////////////////////
    public :

    bool keyPressed( const OIS::KeyEvent &arg );
    bool keyReleased( const OIS::KeyEvent &arg );
	bool mouseMoved( const OIS::MouseEvent &arg );
    bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
    bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
    Ogre::SceneManager *getSceneManager(void) { return mSceneMgr; }

    private :

	void createScene(void);
    void SetupGame(void);

    //Mouse picking
	BallGameEntity *LastHighligted;
    //////////////////////////////////////////////////


	/////////////////// CE GUI ////////////////////////

    private :

    void SetupGUI(void);
    bool EnteringArea(const CEGUI::EventArgs &e);
    bool LeavingArea(const CEGUI::EventArgs &e);
    template<typename T> T*CreateNewGUIComponent(std::string &TypeName, std::string &Name = "");
    template<typename T> T*CreateNewGUIComponent(const char *TypeName, const char *Name = "");

    CEGUI::OgreRenderer* mRenderer;

    CEGUI::LayoutContainer* MainLayout;
    CEGUI::Titlebar *LevelNameBanner;

    // Import Level Buttons
    std::list<CEGUI::Window*> ImportLevelButtons;

    CEGUI::Combobox *ChooseLevelToImportComboB;
    bool ChooseLevelToImportComboBCallback(const CEGUI::EventArgs &e);
    CEGUI::PushButton *ImportLevelPushB;
    bool ImportLevelPushBCallback(const CEGUI::EventArgs &e);
    CEGUI::Window *ImportLevelWindow;
    void BuildImportLevelWindowContent(Node *parent);

    //Main Menu Buttons
    std::list<CEGUI::Window*> MainMenuButtons;

    CEGUI::PushButton *StopPhysicPushB;
    bool StopPhysicPushBCallback(const CEGUI::EventArgs &e);
    CEGUI::PushButton *EditModePushB;
    bool EditModePushBCallback(const CEGUI::EventArgs &e);
    CEGUI::PushButton *StatesModePushB;
    bool StatesModePushBCallback(const CEGUI::EventArgs &e);
    CEGUI::Combobox *ChooseLevelComboB;
    bool ChooseLevelComboBCallback(const CEGUI::EventArgs &e);
    CEGUI::Editbox *NewLevelEditB;
    CEGUI::PushButton *NewLevelCreateB;
    bool NewLevelCreateBCallback(const CEGUI::EventArgs &e);
    CEGUI::PushButton *SaveLevelPushB;
    bool SaveLevelPushBCallback(const CEGUI::EventArgs &e);
    CEGUI::PushButton *QuitPushB;
    bool QuitPushBCallback(const CEGUI::EventArgs &e);

    //Edit buttons

    //States Buttons
    std::list<CEGUI::Window*> StatesButtons;
    CEGUI::Titlebar *StatesBanner;
    CEGUI::PushButton *SaveStatePushB;
    void SetupStatesButtons(void);
    bool SaveStatePushBCallback(const CEGUI::EventArgs &e);
    CEGUI::Combobox *ChooseStateToLoadB;
    bool ChooseStateToLoadBCallback(const CEGUI::EventArgs &e);
    CEGUI::PushButton *LoadStatePushB;
    bool LoadStatePushBCallback(const CEGUI::EventArgs &e);
    CEGUI::PushButton *DelStatePushB;
    bool DelStatePushBCallback(const CEGUI::EventArgs &e);

    //Add new elements Buttons & Callbacks
    std::list<CEGUI::Window*> EditButtons;
    CEGUI::PushButton *ImportLevelActivateInterfacePushB;
    bool ImportLevelActivateInterfacePushBCallback(const CEGUI::EventArgs &e);
    CEGUI::Titlebar *EditingModeTitleBanner;
    CEGUI::Titlebar *AddElementTitleBanner;
    CEGUI::Combobox *ChooseTypeOfElementToAddB;
    bool ChooseTypeOfElementToAddBCallback(const CEGUI::EventArgs &e);
    CEGUI::Window *ThumbnailWindow;
    void CreateThumbnail(String meshname);
    CEGUI::PushButton *PlaceNewElementB;
    bool PlaceNewElementBCallback(const CEGUI::EventArgs &e);
    CEGUI::PushButton *EditElementB;
    bool EditElementBCallback(const CEGUI::EventArgs &e);
    CEGUI::PushButton *DeleteElementB;
    bool DeleteElementBCallback(const CEGUI::EventArgs &e);
    CEGUI::PushButton *MoveElementB;
    bool MoveElementBCallback(const CEGUI::EventArgs &e);
    void SetMoveNewElement(void);
    void SetMoveElement(void);
    CEGUI::PushButton *RotateElementB;
    bool RotateElementBCallback(const CEGUI::EventArgs &e);
    void SetRotateNewElement(void);
    void SetRotateElement(void);
    CEGUI::PushButton *ScaleElementB;
    bool ScaleElementBCallback(const CEGUI::EventArgs &e);
    void SetScaleNewElement(void);
    void SetScaleElement(void);
    CEGUI::ToggleButton *GroupElementsB;
    bool GroupElementsBCallback(const CEGUI::EventArgs &e);


    //Edit Ball Buttons & Callbacks
    std::list<CEGUI::Window*> EditBallButtons;
    CEGUI::Editbox *BallMassValueEditB;
    CEGUI::PushButton *ApplyMassChangesToBallPushB;
    bool ApplyMassChangesToBallPushBCallback(const CEGUI::EventArgs &event);

    //Edit Case Buttons & Callbacks
    std::list<CEGUI::Window*> EditCaseButtons;
    CEGUI::ToggleButton *CaseHasForceToggleB;
    bool CaseHasForceToggleBCallback(const CEGUI::EventArgs &e);
    CEGUI::Editbox *CaseForceValueEditB;
    bool CaseForceValueEditBCallback(const CEGUI::EventArgs &event);
    CEGUI::ToggleButton *CaseHasForceDirectionToggleB;
    bool CaseHasForceDirectionToggleBCallback(const CEGUI::EventArgs &event);
    CEGUI::Editbox *CaseForceDirectionXValueEditB;
    bool CaseForceDirectionXValueEditBMouseWheelCallback(const CEGUI::EventArgs &event);
    CEGUI::Editbox *CaseForceDirectionYValueEditB;
    bool CaseForceDirectionYValueEditBMouseWheelCallback(const CEGUI::EventArgs &event);
    CEGUI::Editbox *CaseForceDirectionZValueEditB;
    bool CaseForceDirectionZValueEditBMouseWheelCallback(const CEGUI::EventArgs &event);
    CEGUI::PushButton *NormalizeCaseForceDirectionPushB;
    bool NormalizeCaseForceDirectionPushBCallback(const CEGUI::EventArgs &e);
    void NormalizeForceDirection(void);
    CEGUI::PushButton *ApplyForceChangesToCasePushB;
    bool ApplyForceChangesToCasePushBCallback(const CEGUI::EventArgs &event);


	//////////////////////////////////////////////////

    /////////////////// RapidJson ////////////////////

    void ExportLevelIntoJson(String &export_str);

    //////////////////////////////////////////////////

    virtual bool frameEnded(const Ogre::FrameEvent& fe);
    void SetCam(float x, float y, float z);
    void MoveCam(float x, float y, float z);
};


#endif /* BALLGAME_H_ */
