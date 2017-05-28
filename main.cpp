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

//Objets � hitbox
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
CDXUTDialog g_SampleUI;		//Dialogue pour les controles sp�cifiques (controles au sens par exemple des touches cheloux)
CD3DSettingsDlg g_SettingsDlg; //Parametre du device principale

XMVECTOR g_CameraOrigins[CAMERA_COUNT]; //Liste des emplacements de la cam�ra

//Objets primaires (les gros quoi)
BoundingFrustum g_PrimaryFustrum;	//Frustrum : Cone (� base circulaire ou car�e) sans pointe du coup le sommet est plat. Vu de cot� : Trap�ze
BoundingOrientedBox g_PrimaryOrientedBox;
BoundingBox g_PrimaryAABox;
CollisionRay g_PrimaryRay;

//Objets secondaires (les petits)
CollisionSphere     g_SecondarySpheres[GROUP_COUNT];
CollisionBox        g_SecondaryOrientedBoxes[GROUP_COUNT];
CollisionAABox      g_SecondaryAABoxes[GROUP_COUNT];
CollisionTriangle   g_SecondaryTriangles[GROUP_COUNT];

//Contr�les cheloux (UI control)
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



/*
	WINAPI : Entr�e de l'API windows (fonction main() mais pour windows)
	hInstance : Instance principale du programme : genre de conteneur qui repr�sente le programme et tout ce qu'il contien
	hPrevInstance : Argument anciennement utilis� (Windows 16bits) donc toujours == NULL
	lpCmdLine : Les arguments de la commande si le programme est d�mar� via cmd ( ex: Si le programme s'appelle 'pgrm' et que l'on �x�cute cette ligne: "pgrm --help --version" alors lpCmdLine=L"--help --version"
	nCmdShow : Le mode d'affichage de

*/
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {

	//V�rification de la m�moire lors du run
#if defined(DEBUG)|defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	//DirectX utilise les instructions SSE2 sur Windows. On v�rifi donc qu'elle sont bien support�es le plus t�t possible
	if (!XMVerifyCPUSupport()) {
		MessageBox(NULL, TEXT("Cette application n�c�ssite le support des instructions processeur SSE2."), TEXT("Core"), MB_OK | MB_ICONEXCLAMATION);
		return -1;
	}

	//Configure les fonctions que doit appeler directX pour ses diff�rentes actions
	DXUTSetCallbackMsgProc(MsgProc);
	DXUTSetCallbackKeyboard(OnKeyboard);


}

/*
	Fait bouger tous les objets de la sc�ne
	fTime : Temps actuel (InGame)
*/

void Animate(double fTime) {
	float t = (FLOAT)(fTime*0.2);
	const float camera0OriginX = XMVectorGetX(g_CameraOrigins[0]);
	const float camera1OriginX = XMVectorGetX(g_CameraOrigins[1]);
	const float camera2OriginX = XMVectorGetX(g_CameraOrigins[2]);
	const float camera3OriginX = XMVectorGetX(g_CameraOrigins[3]);
	const float camera3OriginZ = XMVectorGetZ(g_CameraOrigins[3]);


}

/*
	Cette fonction est la fonction o� toutes les procedures li�es � la fen�tre sont g�r�es
	hWnd : Fen�tre principale
	uMsg : Message envoy� par la boucle �v�nementielle (boucle principale du programme)
	wParam & lParam : Pr�cisions sur l'origine du message

	LRESULT : La valeur retourn�e d�pend (son type aussi) du message � process
	CALLBACK : Pr�cise que la fonction doit �tre appel�e de la mani�re la plus standard possible (__stdcall ici -> de droite � gauche et de haut en bas
*/
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext) {

	return 0;
}

/*
	Cette fonction met la camera sur une vue particuli�re au groupe d'objet
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
	Cette fonction est la pour capter ce qui sort du clavier donc surtout les touches efonc�es ou non
	nChar : Num�ro de la touche
	bKeyDown : si la touche est appuy�
	bAltDown : si la touche ALT est appuy� en m�me temps que la touche
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
	Cette fonction est appel�e si on clique sur un des bouttons du GUI
	nEvent : Num�ro de l'event (Dans notre cas, on ne traite que les cliques donc on ignore ce param�tre)
	nControlID : Num�ro du boutton
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
	Cette fonction met � jour la sc�ne. Son nom peut changer en fonction de l'API D3D utilis�e
	fTime : Temps actuel (InGame)
	fEnlapsedTime : Temps depuis la derni�re update
*/
void CALLBACK OnFrameMove(double fTime, float fEnlapsedTime, void* pUserContext) {

}