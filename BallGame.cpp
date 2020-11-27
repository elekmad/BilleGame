/*
 * BallGame.cpp
 *
 *  Created on: 15 nov. 2020
 *      Author: damien
 */

#include "BallGame.h"
#include <PhysicsUtils.h>
#include <vector>
#include <map>
#include <iostream>
#include <dNewtonScopeBuffer.h>
#include <Newton.h>
#include <OgreRay.h>


BallGameEntity::BallGameEntity(const dMatrix& matrix) : m_matrix(matrix)
	,m_curPosition (matrix.m_posit)
	,m_nextPosition (matrix.m_posit)
	,m_curRotation (dQuaternion (matrix))
	,m_nextRotation (dQuaternion (matrix))
{
	OgreEntity = NULL;
}

void BallGameEntity::SetMatrixUsafe(const dQuaternion& rotation, const dVector& position)
{
	m_curPosition = m_nextPosition;
	m_curRotation = m_nextRotation;

	m_nextPosition = position;
	m_nextRotation = rotation;

	dFloat angle = m_curRotation.DotProduct(m_nextRotation);
	if (angle < 0.0f) {
		m_curRotation.Scale(-1.0f);
		std::cout << "angle < 0 : " << angle << std::endl;
	}
}

void BallGameEntity::TransformCallback(const NewtonBody* body, const dFloat* matrix, int threadIndex)
{
	BallGameEntity* const ent = (BallGameEntity*) NewtonBodyGetUserData(body);
	if (ent) {
		BallGame* const scene = (BallGame*)NewtonWorldGetUserData(NewtonBodyGetWorld(body));
		dMatrix transform(matrix);
		dQuaternion rot;
		NewtonBodyGetRotation(body, &rot.m_x);

		//scene->Lock(ent->m_lock);
		ent->SetMatrixUsafe(rot, transform.m_posit);
		//scene->Unlock(ent->m_lock);
//		std::cout << "Entity transform " << "position {" << ent->m_curPosition.m_x << ", " << ent->m_curPosition.m_y << ", " << ent->m_curPosition.m_z << "}";
//		std::cout << " Orientation {" << ent->m_curRotation.m_w << ", " << ent->m_curRotation.m_x << ", " << ent->m_curRotation.m_y << ", " << ent->m_curRotation.m_z << "}" << std::endl;
		ent->OgreEntity->setPosition(ent->m_curPosition.m_x, ent->m_curPosition.m_y, ent->m_curPosition.m_z);
		ent->OgreEntity->setOrientation(ent->m_curRotation.m_w, ent->m_curRotation.m_x, ent->m_curRotation.m_y, ent->m_curRotation.m_z);
	}
}

void BallEntity::AddForceVector(dVector *force)
{
	std::cout << "Add Force {" << (*force)[0] << ", " << (*force)[1] << ", " << (*force)[2] << "} On ball" << std::endl;
	Forces.Append(force);
}

dVector *BallEntity::GetForceVector()
{
	dVector *ret = NULL;
	dList<dVector*>::dListNode *node = Forces.GetFirst();
	if(node != NULL)
	{
		ret = node->GetInfo();
		Forces.Remove(node);
	}
	return ret;
}

CaseEntity::CaseEntity(const dMatrix& matrix, enum CaseType _type):BallGameEntity(matrix)
{
	type = _type;
	force_to_apply = NAN;
	force_direction = NULL;
}

void CaseEntity::SetForceToApply(float force, dVector *direction)
{
	force_to_apply = force;
	if(force_direction != NULL)
		delete force_direction;
	force_direction = direction;
}

void CaseEntity::AddBallColliding(NewtonBody *ball)
{
	if(ball == NULL)
		return;
	int id = NewtonBodyGetID(ball), size = BallsUnderCollide.GetSize();
	while(size < id)
		BallsUnderCollide[size++] = NULL;
	BallsUnderCollide[id] = ball;
	size = id + 1;
	while(size < BallsUnderCollide.GetSize())
		BallsUnderCollide[size++] = NULL;
}

bool CaseEntity::CheckIfAlreadyColliding(NewtonBody *ball)
{
	bool ret = false;
	if(ball == NULL)
		return ret;
	int id = NewtonBodyGetID(ball);
	if(id < BallsUnderCollide.GetSize())
	{
		if(BallsUnderCollide[id] == ball)
			ret = true;
	}
	return ret;
}

void CaseEntity::ApplyForceOnBall(NewtonBody *ball)
{
	BallEntity *BEntity = (BallEntity*)NewtonBodyGetUserData(ball);
	if(!isnanf(force_to_apply))
	{
		if(force_direction != NULL)
		{
			dVector *force = new dVector();
			*force = force_direction->Scale(force_to_apply);
			BEntity->AddForceVector(force);
		}
		else
		{
			dFloat velocity[3], sum;
			NewtonBodyGetVelocity(ball, velocity);
			sum = velocity[0] + velocity[1] + velocity[2];
			velocity[0] /= sum;
			velocity[1] /= sum;
			velocity[2] /= sum;//Like that we have normalization of velocity into percents, we can use it to scale force.
			dVector *force = new dVector(velocity[0], velocity[1], velocity[2]);
			*force = force->Scale(force_to_apply);
			BEntity->AddForceVector(force);
		}
	}
	AddBallColliding(ball);
}

NewtonWorld* BallGame::GetNewton(void)
{
	return m_world;
}

void BallGame::SetCam(int x, int y, int z)
{
    camx = x;
    camy = y;
    camz = z;
    //camNode->translate(camx, camx, camz, Ogre::Node::TransformSpace::TS_LOCAL);
    mCamera->setPosition(camx, camy, camz);
    GetCamParams();
}

void BallGame::MoveCam(int x, int y, int z)
{
	mCamera->moveRelative(Ogre::Vector3(x, y, z));
    //SetCam(camx + x, camy + y, camz + z);
}


BallGame::BallGame() :
		m_asynchronousPhysicsUpdate(false)
		,m_suspendPhysicsUpdate(false)
		,m_physicsFramesCount(0)
		,m_microsecunds(0)
		,m_mainThreadPhysicsTime(0.0f)
		,m_mainThreadPhysicsTimeAcc(0.0f)
{
    camx = camy = camz = 0;

	m_world = NULL;
	mWindow = NULL;
	LastHighligted = NULL;
	mode = Running;
	// create the newton world
	SetupNewton();
}

void BallGame::SetupNewton(void)
{
	m_world = NewtonCreate();

	// link the work with this user data
	NewtonWorldSetUserData(m_world, this);

	// set a post update callback which is call after all simulation and all listeners updates
	NewtonSetPostUpdateCallback (m_world, PostUpdateCallback);
	NewtonSetNumberOfSubsteps (m_world, MAX_PHYSICS_SUB_STEPS);

	// register contact creation destruction callbacks
	//NewtonWorldSetCreateDestroyContactCallback(m_world, OnCreateContact, OnDestroyContact);
	NewtonLoadPlugins(m_world, "newtonPlugins");
}

BallGame::~BallGame()
{
	if(m_world != NULL)
		NewtonDestroy(m_world);
}

void BallGame::PostUpdateCallback(const NewtonWorld* const world, dFloat timestep)
{
/*	BallGame* const scene = (BallGame*) NewtonWorldGetUserData(world);
	scene->m_cameraManager->FixUpdate(scene->GetNewton(), timestep);
	if (scene->m_updateCamera) {
		scene->m_updateCamera(scene, scene->m_updateCameraContext, timestep);
	}*/
}

void BallGame::UpdatePhysics(dFloat timestep)
{
	// update the physics
//	std::cout << " Update Time " << timestep << std::endl;
	if (m_world && !m_suspendPhysicsUpdate) {
		D_TRACKTIME();

		dFloat timestepInSecunds = 1.0f / MAX_PHYSICS_FPS;
		unsigned64 timestepMicrosecunds = unsigned64 (timestepInSecunds * 1000000.0f);

		unsigned64 currentTime = dGetTimeInMicrosenconds ();
		unsigned64 nextTime = currentTime - m_microsecunds;
		if (nextTime > timestepMicrosecunds * 2) {
			m_microsecunds = currentTime - timestepMicrosecunds * 2;
			nextTime = currentTime - m_microsecunds;
		}

		bool newUpdate = false;
		dFloat physicsTime = 0.0f;
		//while (nextTime >= timestepMicrosecunds)
		if (nextTime >= timestepMicrosecunds)
		{
			newUpdate = true;
			ClearDebugDisplay(m_world);

			CheckforCollides();

#ifdef DEMO_CHECK_ASYN_UPDATE
			g_checkAsyncUpdate = 1;
#endif
			if (m_asynchronousPhysicsUpdate) {
				NewtonUpdateAsync(m_world, timestepInSecunds);
#ifdef DEMO_CHECK_ASYN_UPDATE
				NewtonWaitForUpdateToFinish(m_world);
				g_checkAsyncUpdate = 0;
#endif
			} else {
				NewtonUpdate(m_world, timestepInSecunds);
			}

			physicsTime += NewtonGetLastUpdateTime(m_world);

			nextTime -= timestepMicrosecunds;
			m_microsecunds += timestepMicrosecunds;
		}

		if (newUpdate) {
			m_physicsFramesCount ++;
			m_mainThreadPhysicsTimeAcc += physicsTime;
			if (m_physicsFramesCount >= 16) {
				m_mainThreadPhysicsTime = m_mainThreadPhysicsTimeAcc / m_physicsFramesCount;
				m_physicsFramesCount = 0;
				m_mainThreadPhysicsTimeAcc = 0.0f;
			}

		}

//dTrace (("%f\n", m_mainThreadPhysicsTime));
	}
}

void BallGame::SetupGame(void)
{
//    // register our scene with the RTSS
//    RTShader::ShaderGenerator* shadergen = RTShader::ShaderGenerator::getSingletonPtr();
//    shadergen->addSceneManager(scnMgr);

    // -- tutorial section start --
    //! [turnlights]
	mSceneMgr->setAmbientLight(ColourValue(0.5, 0.5, 0.5));
    //! [turnlights]

    //! [newlight]
    Light* light = mSceneMgr->createLight("MainLight");
    SceneNode* lightNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    lightNode->attachObject(light);
    //! [newlight]

    //! [lightpos]
    lightNode->setPosition(20, 80, 50);
    //! [lightpos]


    //////////////   ADD CASES ///////////////////

	for(int cmpt = 0; cmpt < WORLD_LENGTH; cmpt++)
    {
		for(int cmpt2 = 0; cmpt2 < WORLD_DEPTH; cmpt2++)
		{
				dVector location;
				dVector tsize;
				NewtonBody *tableBody;
				Entity* ogreEntity;
				enum CaseEntity::CaseType type = CaseEntity::CaseType::typeBox;
				if(cmpt2 == 0 && cmpt == 5)
				{
					ogreEntity = mSceneMgr->createEntity("Rampe.mesh");
					type = CaseEntity::CaseType::typeRamp;
				}
				else
					ogreEntity = mSceneMgr->createEntity("Cube.mesh");
				SceneNode* ogreNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(Vector3(cmpt * 50, cmpt2 * 50, 0));
				ogreNode->setScale(25, 25, 25);
				if(cmpt2 == 0 && cmpt == 5)
				{
					ogreNode->roll(Degree(-90));
					ogreNode->setPosition(cmpt * 50, cmpt2 * 50, 25);
				}
				ogreNode->attachObject(ogreEntity);
				Vector3 pos(ogreNode->getPosition());
				location.m_x = pos.x;
				location.m_y = pos.y;
				location.m_z = pos.z;
				location.m_w = 1;
				Vector3 AABB(ogreEntity->getBoundingBox().getSize()), scale(ogreNode->getScale());
				tsize.m_x = AABB.x * scale.x;
				tsize.m_y = AABB.y * scale.y;
				tsize.m_z = AABB.z * scale.z;
				tsize.m_w = 0.0f;
				//ident_matrix.m_posit = location;
				Quaternion orientation = ogreNode->getOrientation();
				dMatrix casematrix(orientation.getPitch(false).valueRadians(), orientation.getYaw(false).valueRadians(), orientation.getRoll(false).valueRadians(), location);
				NewtonCollision *collision_tree = NULL;
				if(type == CaseEntity::CaseType::typeRamp)
				{
					Matrix4 ident_ogre_matrix = Matrix4::IDENTITY;
					const MeshPtr ptr = ogreEntity->getMesh();
					collision_tree = ParseEntity(m_world, ptr, ident_ogre_matrix);
				}
				tableBody = WorldAddCase(m_world, tsize, 0, casematrix, type, collision_tree);
				CaseEntity *Entity = (CaseEntity*)NewtonBodyGetUserData(tableBody);
				if(cmpt == 0 && cmpt2 == 0)
				{
					dVector *direction = new dVector(1.0, 0.0, 0.0);
					Entity->SetForceToApply(100.0, direction);
						//					dVector *force = new dVector(0.0, 0.0, 1000.0);
				}
				if(cmpt == 4 && cmpt2 == 0)
				{
					Entity->SetForceToApply(100.0, NULL);
				}
				if(cmpt >= 10 && cmpt2 == 0)
				{
					dVector *direction = new dVector(0.0, 0.0, 1.0);
					Entity->SetForceToApply(1000.0, direction);
						//					dVector *force = new dVector(0.0, 0.0, 1000.0);
				}
				Entity->OgreEntity = ogreNode;
				AddCase(tableBody);
		}
    }

    //////////////   ADD BALLS ///////////////////

	for(int cmpt = 0; cmpt < 1; cmpt++)
    {
		for(int cmpt2 = 0; cmpt2 < 1; cmpt2++)
		{
			dVector location;
			dVector tsize;
			NewtonBody *BallBody;
			Entity* ogreEntity = mSceneMgr->createEntity("Sphere.mesh");
			SceneNode* ogreNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(Vector3(cmpt * 50, cmpt2 * 50, 55));
			ogreNode->setScale(25, 25, 25);
			ogreNode->attachObject(ogreEntity);
			Vector3 pos(ogreNode->getPosition());
			location.m_x = pos.x;
			location.m_y = pos.y;
			location.m_z = pos.z;
			location.m_w = 1;
			Vector3 AABB(ogreEntity->getBoundingBox().getSize()), scale(ogreNode->getScale());
			tsize.m_x = AABB.x * scale.x;
			tsize.m_y = AABB.y * scale.y;
			tsize.m_z = AABB.z * scale.z;
			tsize.m_w = 0.0f;
			Quaternion orientation = ogreNode->getOrientation();
			dMatrix ballmatrix(orientation.getPitch(false).valueRadians(), orientation.getYaw(false).valueRadians(), orientation.getRoll(false).valueRadians(), location);
			BallBody = WorldAddBall(m_world, 10, tsize, 0, ballmatrix);
			BallGameEntity *Entity = (BallGameEntity*)NewtonBodyGetUserData(BallBody);
			Entity->OgreEntity = ogreNode;
			AddBall(BallBody);
		}
    }

    //! [cameramove]
    SetCam(-184, -253, 352);
    mCamera->setOrientation(Ogre::Quaternion(0.835422, 0.393051, -0.238709, -0.300998));

    m_suspendPhysicsUpdate = false;
//    m_suspendPhysicsUpdate = true;
}

void BallGame::CheckforCollides(void)
{
	for(int cmpt = 0; cmpt < Balls.GetSize(); cmpt++)
	{
		NewtonBody *ball = Balls[cmpt];
		if(ball == NULL)
			continue;
		for(int cmpt2 = 0; cmpt2 < Cases.GetSize(); cmpt2++)
		{
			NewtonBody *Case = Cases[cmpt2];
			if(Case == NULL)
				continue;
//          if(NewtonBodyFindContact(ball, Case) != NULL)
			if(CheckIfBodiesCollide(ball, Case) != NULL)
			{
				std::cout << ball << " id " << cmpt << " and " << Case << " id " << cmpt2 << " Collides by joints" << std::endl;
			}
			if(DoBodiesCollide(m_world, ball, Case))
			{
				CaseEntity *CEntity = (CaseEntity*)NewtonBodyGetUserData(Case);
				//if(!CEntity->CheckIfAlreadyColliding(ball))
				{
					CEntity->ApplyForceOnBall(ball);

					std::cout << ball << " id " << cmpt << " and " << Case << " id " << cmpt2 << " Collides" << std::endl;
				}
//				else
//					std::cout << ball << " id " << cmpt << " and " << Case << " id " << cmpt2 << " Already Colliding" << std::endl;
			}
		}
	}
}

void BallGame::AddBall(NewtonBody *ball)
{
	if(ball == NULL)
		return;
	int id = NewtonBodyGetID(ball), size = Balls.GetSize();
	while(size < id)
		Balls[size++] = NULL;
	Balls[id] = ball;
	size = id + 1;
	while(size < Balls.GetSize())
		Balls[size++] = NULL;
}

void BallGame::AddCase(NewtonBody *Wcase)
{
	if(Wcase == NULL)
		return;
	int id = NewtonBodyGetID(Wcase), size = Cases.GetSize();
	while(size < id)
		Cases[size++] = NULL;
	Cases[id] = Wcase;
	size = id + 1;
	while(size < Cases.GetSize())
		Cases[size++] = NULL;
}


bool BallGame::frameEnded(const Ogre::FrameEvent& fe)
{
//    std::cout << "Render a frame" << std::endl;
	dFloat timestep = dGetElapsedSeconds();
	UpdatePhysics(timestep);

	for(int cmpt = 0; cmpt < Balls.GetSize(); cmpt++)
	{
		NewtonBody *ball = Balls[cmpt];
		if(ball == NULL)
			continue;
		Vector3 worldPos = ((BallGameEntity*)NewtonBodyGetUserData(ball))->OgreEntity->getPosition();
		Vector3 hcsPosition = mCamera->getProjectionMatrix() * mCamera->getViewMatrix() * worldPos;
#define ECART 0.25
//#define ECART 0.8
		while(hcsPosition.x >= ECART)
		{
			MoveCam(1, 0, 0);
			hcsPosition = mCamera->getProjectionMatrix() * mCamera->getViewMatrix() * worldPos;
		}
		while(hcsPosition.x <= -1 * ECART)
		{
			MoveCam(-1, 0, 0);
			hcsPosition = mCamera->getProjectionMatrix() * mCamera->getViewMatrix() * worldPos;
		}
		while(hcsPosition.y >= ECART)
		{
			MoveCam(0, 1, 0);
			hcsPosition = mCamera->getProjectionMatrix() * mCamera->getViewMatrix() * worldPos;
		}
		while(hcsPosition.y <= - 1 * ECART)
		{
			MoveCam(0, -1, 0);
			hcsPosition = mCamera->getProjectionMatrix() * mCamera->getViewMatrix() * worldPos;
		}
	}

    return true;
}

bool BallGame::mouseMoved(const OIS::MouseEvent &arg)
{
	if(arg.state.Z.rel != 0)
		MoveCam(0,  0,  -10 * arg.state.Z.rel);
	if(mode == Editing)
	{
		if(LastHighligted != NULL)
		{
			LastHighligted->showBoundingBox(false);
			LastHighligted = NULL;
		}
		RaySceneQuery *mRayScanQuery = mSceneMgr->createRayQuery(Ogre::Ray());

		Real x, y;
		x = (float)arg.state.X.abs / (float)mWindow->getWidth();
		y = (float)arg.state.Y.abs / (float)mWindow->getWidth();
		Ray mouseRay = mCamera->getCameraToViewportRay(x, y);
		mRayScanQuery->setRay(mouseRay);
		mRayScanQuery->setSortByDistance(true, 1);
		RaySceneQueryResult &result = mRayScanQuery->execute();
		RaySceneQueryResult::iterator itr = result.begin();
		std::cout << "Picking Mouse : " << result.size() << std::endl;
		while(itr != result.end())
		{
			if(itr->movable != NULL)
			{
				LastHighligted = itr->movable->getParentSceneNode();
				LastHighligted->showBoundingBox(true);
			}
			itr++;
		}
		delete mRayScanQuery;
	}
	return true;
}

bool BallGame::keyPressed(const OIS::KeyEvent &arg)
{
	std::cout << "Key pressed " << arg.key << std::endl;
	BaseApplication::keyPressed(arg);
    switch (arg.key)
    {
	case OIS::KeyCode::KC_ESCAPE :
	    mRoot->queueEndRendering();
	    break;
	case OIS::KeyCode::KC_UP:
	    MoveCam(0, 0, 10);
	    break;
	case OIS::KeyCode::KC_DOWN:
	    MoveCam(0, 0, -10);
	    break;
	case OIS::KeyCode::KC_LEFT:
	    MoveCam(-10, 0, 0);
	    break;
	case OIS::KeyCode::KC_RIGHT:
	    MoveCam(10, 0, 0);
	    break;
//	case OIS::KeyCode::KC_KP_8:
//	    CamPitch(10);
//	    break;
//	case OIS::KeyCode::KC_KP_5:
//	    CamPitch(-10);
//	    break;
//	case OIS::KeyCode::KC_KP_4:
//	    CamYaw(10);
//	    break;
//	case OIS::KeyCode::KC_KP_6:
//	    CamYaw(-10);
//	    break;
//	case OIS::KeyCode::KC_KP_7:
//	    CamRoll(10);
//	    break;
//	case OIS::KeyCode::KC_KP_9:
//	    CamRoll(-10);
//	    break;
        case OIS::KeyCode::KC_PAUSE:
            m_suspendPhysicsUpdate = m_suspendPhysicsUpdate ? false : true;
            break;
        case OIS::KeyCode::KC_F1:
            if(mode == Running)
            	mode = Editing;
			else
				mode = Running;
            break;
    }
    return true;
}
