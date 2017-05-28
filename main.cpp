#include <DXUT.h>
#include <DXUTgui.h>
#include <DXUTcamera.h>
#include <DXUTsettingsdlg.h>
#include <SDKmisc.h>
#include "e:/C++/Projet_Moteur_V2/Projet_Moteur_V2/resource.h" //VS ne veut pas le prendre sans le chemin complet ???

#include <DirectXColors.h>
#include <DirectXCollision.h>

#include "CommonStates.h"
#include "Effects.h"
#include "PrimitiveBatch.h"
#include "VertexTypes.h"

using namespace DirectX;

//Objets à hitbox
struct CollisionSphere
{
	BoundingSphere sphere;
	ContainmentType collision;
};

struct CollisionBox
{
	BoundingOrientedBox obox;
	ContainmentType collision;
};

struct CollisionAABox
{
	BoundingBox aabox;
	ContainmentType collision;
};

struct CollisionFrustum
{
	BoundingFrustum frustum;
	ContainmentType collision;
};

struct CollisionTriangle
{
	XMVECTOR pointa;
	XMVECTOR pointb;
	XMVECTOR pointc;
	ContainmentType collision;
};

struct CollisionRay
{
	XMVECTOR origin;
	XMVECTOR direction;
};

//Constantes
enum {
	GROUP_COUNT = 4,
	CAMERA_COUNT = 4
};
const float CAMERA_SPACING = 50.f;

//Variables globales
CModelViewerCamera g_Camera; //La camera
CDXUTDialogResourceManager g_DialogResourceManager;	//Manager pour les resources partargés des différents dialogs
CDXUTDialog g_SampleUI;		//Dialogue pour les controles spécifiques (controles au sens par exemple des touches cheloux)
CDXUTDialog g_HUD;	//Dialog pour les controles standards
CD3DSettingsDlg g_SettingsDlg; //Parametre du device principale
CDXUTTextHelper* g_pTxtHelper = nullptr;

ID3D11InputLayout* g_pBatchInputLayout = nullptr;

std::unique_ptr<CommonStates>	g_States;
std::unique_ptr<BasicEffect>	g_BatchEffect;
std::unique_ptr<PrimitiveBatch<VertexPositionColor>> g_Batch;

XMVECTOR g_CameraOrigins[CAMERA_COUNT]; //Liste des emplacements de la caméra

//Objets primaires (les gros quoi)
BoundingFrustum				g_PrimaryFustrum;	//Frustrum : Cone (à base circulaire ou carée) sans pointe du coup le sommet est plat. Vu de coté : Trapèze
BoundingOrientedBox			g_PrimaryOrientedBox;
BoundingBox					g_PrimaryAABox;
CollisionRay				g_PrimaryRay;

//Box à l'endroit où le ray coupe un objet
CollisionAABox g_RayHitResultBox;

//Objets secondaires (les petits)
CollisionSphere     g_SecondarySpheres[GROUP_COUNT];
CollisionBox        g_SecondaryOrientedBoxes[GROUP_COUNT];
CollisionAABox      g_SecondaryAABoxes[GROUP_COUNT];
CollisionTriangle   g_SecondaryTriangles[GROUP_COUNT];

//Contrôles cheloux (UI control)
#define IDC_STATIC				-1
#define IDC_TOGGLEFULLSCREEN	1
#define IDC_TOGGLEREF			2
#define IDC_CHANGEDEVICE		3
#define IDC_TOGGLEWARP			4
#define IDC_GROUP				5

//Declarations des fonction pour pouvoir les utiliser dans WinMain
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserConext);
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext);
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext);
void CALLBACK OnFrameMove(double fTime, float fEnlapsedTime, void* pUserContext);
bool CALLBACK ModifyDeviceSettings(DXUTD3D11DeviceSettings* pDeviceSettings, void* pUserContext);
bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext);
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);

void Animate(double fTime);
void Collide();

void SetViewForGroup(int group);


/*
	WINAPI : Entrée de l'API windows (fonction main() mais pour windows)
	hInstance : Instance principale du programme : genre de conteneur qui représente le programme et tout ce qu'il contien
	hPrevInstance : Argument anciennement utilisé (Windows 16bits) donc toujours == NULL
	lpCmdLine : Les arguments de la commande si le programme est démaré via cmd ( ex: Si le programme s'appelle 'pgrm' et que l'on éxécute cette ligne: "pgrm --help --version" alors lpCmdLine=L"--help --version"
	nCmdShow : Le mode d'affichage de

*/
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {

	//Vérification de la mémoire lors du run
#if defined(DEBUG)|defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	//DirectX utilise les instructions SSE2 sur Windows. On vérifi donc qu'elle sont bien supportées le plus tôt possible
	if (!XMVerifyCPUSupport()) {
		MessageBox(NULL, TEXT("Cette application nécéssite le support des instructions processeur SSE2."), TEXT("Core"), MB_OK | MB_ICONEXCLAMATION);
		return -1;
	}

	//Configure les fonctions que doit appeler directX pour ses différentes actions
	DXUTSetCallbackMsgProc(MsgProc);
	DXUTSetCallbackKeyboard(OnKeyboard);


}

/*
	
*/
bool CALLBACK ModifyDeviceSettings(DXUTD3D11DeviceSettings* pDeviceSettings, void* pUserContext) {
	return true;
}

//Elle est acceptable pose pas de question
bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext) {
	return true;
}

/*
	Cette fonction est appelée quand directX est créé
*/
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext) {
	HRESULT hr;

	auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
	V_RETURN(g_DialogResourceManager.OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext));
	V_RETURN(g_SettingsDlg.OnD3D11CreateDevice(pd3dDevice));
	g_pTxtHelper = new CDXUTTextHelper(pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15);

	//Creation d'un autre render
	g_States.reset(new CommonStates(pd3dDevice));
	g_Batch.reset(new PrimitiveBatch<VertexPositionColor>(pd3dImmediateContext));

	g_BatchEffect.reset(new BasicEffect(pd3dDevice));
	g_BatchEffect->SetVertexColorEnabled(true);

	{
		void const* shaderByteCode;
		size_t byteCodeLength;

		g_BatchEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
		hr = pd3dDevice->CreateInputLayout(VertexPositionColor::InputElements, VertexPositionColor::InputElementCount, shaderByteCode, byteCodeLength, &g_pBatchInputLayout);

		if (FAILED(hr))
			return hr;
	}

	//Setup des paramètres de la camera

	auto pComboBox = g_SampleUI.GetComboBox(IDC_GROUP);
	SetViewForGroup((pComboBox) ? (int)PtrToInt(pComboBox->GetSelectedData()) : 0);
	g_HUD.GetButton(IDC_TOGGLEWARP)->SetEnabled(true);

	return S_OK;
}

/*
	Fait bouger tous les objets de la scène
	fTime : Temps actuel (InGame)
*/

void Animate(double fTime) {
	float t = (FLOAT)(fTime*0.2);

	const float camera0OriginX = XMVectorGetX(g_CameraOrigins[0]);
	const float camera1OriginX = XMVectorGetX(g_CameraOrigins[1]);
	const float camera2OriginX = XMVectorGetX(g_CameraOrigins[2]);
	const float camera3OriginX = XMVectorGetX(g_CameraOrigins[3]);
	const float camera3OriginZ = XMVectorGetZ(g_CameraOrigins[3]);

	//Animation de la sphere 0 autour du frustum
	g_SecondarySpheres[0].sphere.Center.x = 10 * sinf(3 * t);
	g_SecondarySpheres[0].sphere.Center.y = 7 * cosf(5 * t);

	//Animation de la oriented box 0 autour du frustum
	g_SecondaryOrientedBoxes[0].obox.Center.x = 8 * sinf(3.5f * t);
	g_SecondaryOrientedBoxes[0].obox.Center.y = 5 * cosf(5.1f * t);
	XMStoreFloat4(&(g_SecondaryOrientedBoxes[0].obox.Orientation), XMQuaternionRotationRollPitchYaw(t * 1.4f,
																									t * 0.2f,
																									t));

	//Animation de la box (Axis-Aligned) autour de l'oriented box
	g_SecondaryAABoxes[2].aabox.Center.x = 8 * sinf(1.3f * t) + camera2OriginX;
	g_SecondaryAABoxes[2].aabox.Center.y = 8 * cosf(5.2f * t);
	g_SecondaryAABoxes[2].aabox.Center.z = 8 * cosf(3.5f * t);

	//Triangles équilateraux
	const XMVECTOR TrianglePointA = { 0, 2, 0, 0 };
	const XMVECTOR TrianglePointB = { 1.732f, -1, 0, 0 };
	const XMVECTOR TrianglePointC = { -1.732f, -1, 0, 0 };

	//Animation du triangle 0 autour du frustum
	XMMATRIX TriangleCoords = XMMatrixRotationRollPitchYaw(t * 1.4f, t * 2.5f, t);
	XMMATRIX Translation = XMMatrixTranslation(5 * sinf(5.3f * t) + camera0OriginX,
											   5 * cosf(2.3f * t),
											   5 * sinf(3.4f * t));
	TriangleCoords = XMMatrixMultiply(TriangleCoords, Translation);
	g_SecondaryTriangles[0].pointa = XMVector3Transform(TrianglePointA, TriangleCoords);
	g_SecondaryTriangles[0].pointb = XMVector3Transform(TrianglePointB, TriangleCoords);
	g_SecondaryTriangles[0].pointc = XMVector3Transform(TrianglePointC, TriangleCoords);

	//Animation du triangle 1 autour de l'oriented box
	TriangleCoords = XMMatrixRotationRollPitchYaw(t * 1.4f, t * 2.5f, t);
	Translation = XMMatrixTranslation(8 * sinf(5.3f * t) + camera1OriginX,
									  8 * cosf(2.3f * t),
									  8 * sinf(3.4f * t));
	TriangleCoords = XMMatrixMultiply(TriangleCoords, Translation);
	g_SecondaryTriangles[1].pointa = XMVector3Transform(TrianglePointA, TriangleCoords);
	g_SecondaryTriangles[1].pointb = XMVector3Transform(TrianglePointB, TriangleCoords);
	g_SecondaryTriangles[1].pointc = XMVector3Transform(TrianglePointC, TriangleCoords);

	// Animation du triangle 2 autour de l'oriented box
	TriangleCoords = XMMatrixRotationRollPitchYaw(t * 1.4f, t * 2.5f, t);
	Translation = XMMatrixTranslation(8 * sinf(5.3f * t) + camera2OriginX,
									  8 * cosf(2.3f * t),
									  8 * sinf(3.4f * t));
	TriangleCoords = XMMatrixMultiply(TriangleCoords, Translation);
	g_SecondaryTriangles[2].pointa = XMVector3Transform(TrianglePointA, TriangleCoords);
	g_SecondaryTriangles[2].pointb = XMVector3Transform(TrianglePointB, TriangleCoords);
	g_SecondaryTriangles[2].pointc = XMVector3Transform(TrianglePointC, TriangleCoords);

	//Animation du ray (Seule objet primaire animé)
	g_PrimaryRay.direction = XMVectorSet(sinf(t * 3), 0, cosf(t * 3), 0);

	//Animation de la sphere 3 autour du ray
	g_SecondarySpheres[3].sphere.Center = XMFLOAT3(camera3OriginX - 3,
												   0.5f * sinf(t * 5),
												   camera3OriginZ);

	//Animation de la box 3 autour du ray
	g_SecondaryAABoxes[3].aabox.Center = XMFLOAT3(camera3OriginX + 3,
												  0.5f * sinf(t * 4),
												  camera3OriginZ);

	//Animation de l'oriented box 3 autour du ray
	g_SecondaryOrientedBoxes[3].obox.Center = XMFLOAT3(camera3OriginX,
													   0.5f * sinf(t * 4.5f),
													   camera3OriginZ + 3);
	XMStoreFloat4(&(g_SecondaryOrientedBoxes[3].obox.Orientation), XMQuaternionRotationRollPitchYaw(t * 0.9f,
																									t * 1.8f,
																									t));

	//Animation du triangle 3 autour du ray
	TriangleCoords = XMMatrixRotationRollPitchYaw(t * 1.4f, t * 2.5f, t);
	Translation = XMMatrixTranslation(camera3OriginX,
									  0.5f * cosf(4.3f * t),
									  camera3OriginZ - 3);
	TriangleCoords = XMMatrixMultiply(TriangleCoords, Translation);
	g_SecondaryTriangles[3].pointa = XMVector3Transform(TrianglePointA, TriangleCoords);
	g_SecondaryTriangles[3].pointb = XMVector3Transform(TrianglePointB, TriangleCoords);
	g_SecondaryTriangles[3].pointc = XMVector3Transform(TrianglePointC, TriangleCoords);
}

/*
	Cette fonction met à jour toutes les collisions pour les tester
*/
void Collide() {

	//Test les collisions entre les objets et le frustum
	g_SecondarySpheres[0].collision			= g_PrimaryFustrum.Contains(g_SecondarySpheres[0].sphere);
	g_SecondaryOrientedBoxes[0].collision	= g_PrimaryFustrum.Contains(g_SecondaryOrientedBoxes[0].obox);
	g_SecondaryAABoxes[0].collision			= g_PrimaryFustrum.Contains(g_SecondaryAABoxes[0].aabox);
	g_SecondaryTriangles[0].collision		= g_PrimaryFustrum.Contains(g_SecondaryTriangles[0].pointa,
																  g_SecondaryTriangles[0].pointb,
																  g_SecondaryTriangles[0].pointc);

	//Test les collisions entre les objets et l'anligned box
	g_SecondarySpheres[1].collision			= g_PrimaryAABox.Contains(g_SecondarySpheres[1].sphere);
	g_SecondaryOrientedBoxes[1].collision	= g_PrimaryAABox.Contains(g_SecondaryOrientedBoxes[1].obox);
	g_SecondaryAABoxes[1].collision			= g_PrimaryAABox.Contains(g_SecondaryAABoxes[1].aabox);
	g_SecondaryTriangles[1].collision		= g_PrimaryAABox.Contains(g_SecondaryTriangles[1].pointa,
																g_SecondaryTriangles[1].pointb,
																g_SecondaryTriangles[1].pointc);

	//Test les collisions entre le objets et la box
	g_SecondarySpheres[2].collision			= g_PrimaryOrientedBox.Contains(g_SecondarySpheres[2].sphere);
	g_SecondaryOrientedBoxes[2].collision	= g_PrimaryOrientedBox.Contains(g_SecondaryOrientedBoxes[2].obox);
	g_SecondaryAABoxes[2].collision			= g_PrimaryOrientedBox.Contains(g_SecondaryAABoxes[2].aabox);
	g_SecondaryTriangles[2].collision		= g_PrimaryOrientedBox.Contains(g_SecondaryTriangles[2].pointa,
																	  g_SecondaryTriangles[2].pointb,
																	  g_SecondaryTriangles[2].pointc);

	//Test les collisions entre les objets et la ray
	float fDistance = -1.0f;

	float fDist;
	if (g_SecondarySpheres[3].sphere.Intersects(g_PrimaryRay.origin, g_PrimaryRay.direction, fDist))
	{
		fDistance = fDist;
		g_SecondarySpheres[3].collision = INTERSECTS;
	}
	else
		g_SecondarySpheres[3].collision = DISJOINT;

	if (g_SecondaryOrientedBoxes[3].obox.Intersects(g_PrimaryRay.origin, g_PrimaryRay.direction, fDist))
	{
		fDistance = fDist;
		g_SecondaryOrientedBoxes[3].collision = INTERSECTS;
	}
	else
		g_SecondaryOrientedBoxes[3].collision = DISJOINT;

	if (g_SecondaryAABoxes[3].aabox.Intersects(g_PrimaryRay.origin, g_PrimaryRay.direction, fDist))
	{
		fDistance = fDist;
		g_SecondaryAABoxes[3].collision = INTERSECTS;
	}
	else
		g_SecondaryAABoxes[3].collision = DISJOINT;

	if (TriangleTests::Intersects(g_PrimaryRay.origin, g_PrimaryRay.direction,
								  g_SecondaryTriangles[3].pointa,
								  g_SecondaryTriangles[3].pointb,
								  g_SecondaryTriangles[3].pointc,
								  fDist))
	{
		fDistance = fDist;
		g_SecondaryTriangles[3].collision = INTERSECTS;
	}
	else
		g_SecondaryTriangles[3].collision = DISJOINT;

	//Si un des test des intersections du ray est bon, fDistance sera positive
	if (fDistance > 0)
	{
		// Le primary ray est supposé normalisé
		XMVECTOR HitLocation = XMVectorMultiplyAdd(g_PrimaryRay.direction, XMVectorReplicate(fDistance),
												   g_PrimaryRay.origin);
		XMStoreFloat3(&g_RayHitResultBox.aabox.Center, HitLocation);
		g_RayHitResultBox.collision = INTERSECTS;
	}
	else
	{
		g_RayHitResultBox.collision = DISJOINT;
	}
}

/*
	Cette fonction est la fonction où toutes les procedures liées à la fenêtre sont gérées
	hWnd : Fenêtre principale
	uMsg : Message envoyé par la boucle événementielle (boucle principale du programme)
	wParam & lParam : Précisions sur l'origine du message

	LRESULT : La valeur retournée dépend (son type aussi) du message à process
	CALLBACK : Précise que la fonction doit être appelée de la manière la plus standard possible (__stdcall ici -> de droite à gauche et de haut en bas)
*/
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext) {

	return 0;
}

/*
	Cette fonction met la camera sur une vue particulière au groupe d'objet
*/

void SetViewForGroup(int group) {
	assert(group < GROUP_COUNT);
	g_Camera.Reset();
	static const XMVECTORF32 s_Offset0 = { 0.f, 20.f, 20.f, 0.f };
	static const XMVECTORF32 s_Offset = { 0.f, 20.f, -20.f, 0.f };
	XMVECTOR vecEye = g_CameraOrigins[group] + ((group == 0) ? s_Offset0 : s_Offset);
	g_Camera.SetViewParams(vecEye, g_CameraOrigins[group]);
	XMFLOAT3 vecAt;
	XMStoreFloat3(&vecAt, g_CameraOrigins[group]);
	g_Camera.SetModelCenter(vecAt);
}


/*
	Cette fonction est la pour capter ce qui sort du clavier donc surtout les touches efoncées ou non
	nChar : Numéro de la touche
	bKeyDown : si la touche est appuyé
	bAltDown : si la touche ALT est appuyé en même temps que la touche
*/

void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext) {
	switch (nChar) {
		case '1':
		case '2':
		case '3':
		case '4': {
				int group = (nChar - '1');
				auto pComboBox = g_SampleUI.GetComboBox(IDC_GROUP);
				assert(pComboBox != NULL);
				pComboBox->SetSelectedByData(IntToPtr(group));
				SetViewForGroup(group);
			}
				  break;
	}
}
/*
	Cette fonction est appelée si on clique sur un des bouttons du GUI
	nEvent : Numéro de l'event (Dans notre cas, on ne traite que les cliques donc on ignore ce paramètre)
	nControlID : Numéro du boutton
	pControl : Pointeur pour controller le device
*/
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext) {
	switch (nControlID)
	{
		case IDC_TOGGLEFULLSCREEN:
			DXUTToggleFullScreen();
			break;
		case IDC_TOGGLEREF:
			DXUTToggleREF();
			break;
		case IDC_TOGGLEWARP:
			DXUTToggleWARP();
			break;
		case IDC_CHANGEDEVICE:
			g_SettingsDlg.SetActive(!g_SettingsDlg.IsActive());
			break;
		case IDC_GROUP: {
				auto pComboBox = reinterpret_cast<CDXUTComboBox*>(pControl);
				SetViewForGroup((int)PtrToInt(pComboBox->GetSelectedData()));
			}
						break;
	}
}

/*
	Cette fonction met à jour la scène. Son nom peut changer en fonction de l'API D3D utilisée
	fTime : Temps actuel (InGame)
	fEnlapsedTime : Temps depuis la dernière update
*/
void CALLBACK OnFrameMove(double fTime, float fEnlapsedTime, void* pUserContext) {
	//Mise à jour des positions des objets
	Animate(fTime);

	//Mise à jour des collisions
	Collide();

	//Mise à jour de la camera
	g_Camera.FrameMove(fEnlapsedTime);
}