// BingBlur.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#define CacheDir "\\cache"
#define XMLUrl "http://cn.bing.com/HPImageArchive.aspx?format=xml&idx=0&n=1"

typedef struct BlurZoom {
	int BlurZoomPostion;
	struct BlurZoom* Next;
}BlurZoom, *pBlurZoom;

std::string dir;
std::string nDate;

std::string & StringReplaceALL(std::string& str, const std::string& old_value, const std::string& new_value)
{
	for (std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length())
	{
		if ((pos = str.find(old_value, pos)) != std::string::npos)
			str.replace(pos, old_value.length(), new_value);
		else   break;
	}
	return str;
}

void GetTime() {
	struct tm ntm;
	time_t now;
	time(&now);
	localtime_s(&ntm, &now);
	std::stringstream Date;
	Date << ntm.tm_year + 1900 << "_" << ntm.tm_mon + 1 << "_" << ntm.tm_mday;
	nDate = Date.str();
}

std::string GetPicUrl() {
	std::string LocalXmlFullPath = dir + CacheDir + "\\" + nDate + ".xml";
	tinyxml2::XMLDocument doc;
	DeleteUrlCacheEntryA(XMLUrl);
	URLDownloadToFileA(NULL, XMLUrl, LocalXmlFullPath.c_str(), 0, NULL);
	if (tinyxml2::XML_SUCCESS != doc.LoadFile(LocalXmlFullPath.c_str()))return "";
	tinyxml2::XMLElement *images = doc.RootElement();
	tinyxml2::XMLElement *image = images->FirstChildElement("image");
	std::string WebPicUrl = "", WebPicFullUrl;
	if (image != NULL)
		WebPicUrl = image->FirstChildElement("url")->GetText();
	if (WebPicUrl.find(".jpg") == -1)return "";
	WebPicFullUrl = "http://cn.bing.com" + WebPicUrl;
	return WebPicFullUrl;
}

std::string GetPicFile(std::string WebPicFullUrl) {
	std::string LocalPicFullPath = dir + CacheDir + "\\" + nDate + ".jpg";
	StringReplaceALL(WebPicFullUrl, "1366x768", "1920x1080");//GetLargePic
	if (URLDownloadToFileA(NULL, WebPicFullUrl.c_str(), LocalPicFullPath.c_str(), 0, NULL) == S_OK)return LocalPicFullPath;
	return "";
}

int MakeCache(std::string LocalPicFullPath) {
	//RectConfig
	int BlurNum, BlurNumI, PostionNum;
	std::string ConfigDir = dir + "\\config.ini";
	BlurNum = GetPrivateProfileIntA("RectProcess", "BlurNum", 0, ConfigDir.c_str());
	//if (BlurNum == 0)return 1;
	pBlurZoom pBlurZoomHead, pNew = (pBlurZoom)malloc(sizeof(BlurZoom));
	pBlurZoomHead = pNew;
	std::stringstream BlurKeyS;
	std::string BlurKey;
	for (BlurNumI = 0;BlurNumI < BlurNum;BlurNumI++) {
		for (PostionNum = 1;PostionNum < 6;PostionNum++) {
			BlurKeyS.str("");
			BlurKeyS << "BlurZoom" << BlurNumI << PostionNum;
			BlurKey = BlurKeyS.str();
			pNew->BlurZoomPostion = GetPrivateProfileIntA("RectProcess", BlurKey.c_str(), -1, ConfigDir.c_str());
			if (pNew->BlurZoomPostion == -1)return 0;
			pNew->Next = (pBlurZoom)malloc(sizeof(BlurZoom));
			pNew = pNew->Next;
		}//x1,x2,y1,y2,s;
	}pNew->Next = NULL;

	//ResolutionProcess
	cv::Mat image= cv::imread(LocalPicFullPath.c_str()),imROT,imLogoin;
	int Screen_Width = GetSystemMetrics(SM_CXSCREEN),Screen_Height = GetSystemMetrics(SM_CYSCREEN);
	if (Screen_Width / (float)Screen_Height > 16.0 / 9.0) {
		cv::resize(image, image, cv::Size(Screen_Width, (int)(Screen_Width / 1920.0 * 1080)), 0, 0, cv::INTER_AREA);
		image = image(cv::Rect(0, (int)((Screen_Width / 1920.0 * 1080-Screen_Height)/2.0), Screen_Width, Screen_Height));
	}else if(Screen_Width / (float)Screen_Height < 16.0 / 9.0) {
		cv::resize(image, image, cv::Size((int)(Screen_Height / 1080.0 * 1920),Screen_Height) , 0, 0, cv::INTER_AREA);
		image = image(cv::Rect((int)((Screen_Height / 1080.0 * 1920-Screen_Width)/2.0), 0, Screen_Width, Screen_Height));
	}
	//LogoinIm
	imLogoin = image.clone();
	cv::GaussianBlur(imLogoin, imLogoin,cv::Size(81,81),0,0);
	cv::GaussianBlur(imLogoin, imLogoin,cv::Size(81,81),0,0);
	cv::GaussianBlur(imLogoin, imLogoin,cv::Size(81,81),0,0);
	int postion[5];
	std::vector<int> compression_params;
	compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
	compression_params.push_back(95); 
	imwrite(dir + CacheDir + "\\" + "backgroundDefault.jpg", imLogoin,compression_params);
	imLogoin.release();
	//RectProcess
	pNew = pBlurZoomHead;
	for (BlurNumI = 0;BlurNumI < BlurNum;BlurNumI++) {
		for (PostionNum = 0;PostionNum < 5;PostionNum++) {
			postion[PostionNum] = pNew->BlurZoomPostion;pNew = pNew->Next;
		}
		imROT = image(cv::Rect(postion[0], postion[2], postion[1] - postion[0], postion[3] - postion[2]));
		cv::GaussianBlur(imROT, imROT, cv::Size(postion[4] / 2 * 2 + 1, postion[4] / 2 * 2 + 1), 0, 0);
		cv::GaussianBlur(imROT, imROT, cv::Size(postion[4] / 2 * 2 + 1, postion[4] / 2 * 2 + 1), 0, 0);
		cv::GaussianBlur(imROT, imROT, cv::Size(postion[4] / 2 * 2 + 1, postion[4] / 2 * 2 + 1), 0, 0);
	}
	//MaskProcess
	int BlurColor;
	BlurColor = GetPrivateProfileIntA("MaskProcess", "BlurColor", -1, ConfigDir.c_str());
	while (BlurColor == 0 || BlurColor == 1) {
		char MaskPic[MAX_PATH];int Threshold,BlurLevel;
		Threshold = GetPrivateProfileIntA("MaskProcess","Threshold",-1,ConfigDir.c_str());
		if (Threshold == -1)break;
		BlurLevel = GetPrivateProfileIntA("MaskProcess", "BlurLevel", -1, ConfigDir.c_str());
		if (BlurLevel == -1)break;
		GetPrivateProfileStringA("MaskProcess", "MaskPic", "", MaskPic, MAX_PATH, ConfigDir.c_str());
		if (MaskPic == "")break;
		std::string MaskPicS = dir + MaskPic;
		cv::Mat mask = cv::imread(MaskPicS.c_str());
		cv::cvtColor(mask, mask, CV_RGB2GRAY);
		if (BlurColor == 0) {
			cv::threshold(mask, mask, Threshold, 255, 0);
			//bitwise_not(mask, mask);
		}
		else
			cv::threshold(mask, mask, Threshold, 255, 1);
		cv::Mat imMaskBlur;
		imMaskBlur = image.clone();
		cv::GaussianBlur(imMaskBlur, imMaskBlur, cv::Size(BlurLevel / 2 * 2 + 1, BlurLevel / 2 * 2 + 1), 0, 0);
		cv::GaussianBlur(imMaskBlur, imMaskBlur, cv::Size(BlurLevel / 2 * 2 + 1, BlurLevel / 2 * 2 + 1), 0, 0);
		cv::GaussianBlur(imMaskBlur, imMaskBlur, cv::Size(BlurLevel / 2 * 2 + 1, BlurLevel / 2 * 2 + 1), 0, 0);
		imMaskBlur.copyTo(image,mask);
		break;
	}
	imwrite(dir + CacheDir + "\\" + "wallpaper.bmp", image);
	std::vector<int> compression_params_cache;
	compression_params_cache.push_back(CV_IMWRITE_JPEG_QUALITY);
	compression_params_cache.push_back(100);
	imwrite(dir + CacheDir + "\\" + "TranscodedWallpaper.jpg", image, compression_params);
	image.release();
	imROT.release();
	return 1;//Porcess 1 Pic;
}

int SetWallPaper(std::string LocalPicFullpath) {
	char Path[MAX_PATH];
	SHGetSpecialFolderPathA(0,Path, CSIDL_MYPICTURES, 0);
	std::string DesPath = Path;
	DesPath += "\\BingBlur";
	CreateDirectoryA(DesPath.c_str(),NULL);
	DesPath += "\\wallpaper.bmp";
	DeleteFileA(DesPath.c_str());
	std::string BMP = LocalPicFullpath + "\\" + "wallpaper.bmp";
	CopyFileA(BMP.c_str(),DesPath.c_str(),FALSE);
	SHGetSpecialFolderPathA(0, Path, CSIDL_APPDATA, 0);
	DesPath = Path;
	DesPath += "\\Microsoft\\Windows\\Themes\\TranscodedWallpaper.jpg";
	std::string JPG = LocalPicFullpath + "\\" + "TranscodedWallpaper.jpg";
	CopyFileA(JPG.c_str(), DesPath.c_str(), FALSE);
	bool result = false;
	result = SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (PVOID)DesPath.c_str(), 0);
	if (result == false) return -1;
	else SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (PVOID)DesPath.c_str(), SPIF_SENDCHANGE);
	return 0;
}
void SetLoginPaper(std::string LocalSmallPicFullpath) {
	char path[MAX_PATH];
	SHGetSpecialFolderPathA(0, path, CSIDL_SYSTEM, 0);
	std::string SourceFile = dir + CacheDir + "\\backgroundDefault.jpg", DesFile = path;
	DesFile += ("\\oobe\\info");
	CreateDirectoryA(DesFile.c_str(), NULL);
	DesFile += ("\\backgrounds");
	CreateDirectoryA(DesFile.c_str(), NULL);
	DesFile += ("\\backgroundDefault.jpg");
	DeleteFileA(DesFile.c_str());
	CopyFileA(SourceFile.c_str(), DesFile.c_str(), FALSE);
}

int main(int argc, char** argv)
{
	GetTime();
	dir = argv[0];
	dir = dir.substr(0, dir.find_last_of("\\"));
	std::string str = dir + CacheDir;
	SHFILEOPSTRUCTA CacheDirOp = { 0 };
	CacheDirOp.fFlags = FOF_NO_UI;
	CacheDirOp.pFrom = str.c_str();
	CacheDirOp.pTo = NULL;
	CacheDirOp.wFunc = FO_DELETE;
	if (SHFileOperationA(&CacheDirOp) != 0) {
		std::string cmd = "CMD /C DEL "+str+" /q";
		WinExec((LPCSTR)cmd.c_str(), SW_HIDE);
	}
	CreateDirectoryA(str.c_str(), NULL);
	str = dir + "\\config.ini";
	if (GetPrivateProfileIntA("RectProcess", "BlurNum", -1, str.c_str()) == -1) {
		FILE *stream;
		fopen_s(&stream, str.c_str(), "w+t");
		char msg[] = ";BingBlur ByMelo@melo.site\r\n[LoginPic]\r\n;是否启用登陆背景\r\nStatue= 1\r\n\r\n[RectProcess]\r\n;矩形块模糊\r\n;当BlurNum为1时,BlurZoom0x区有效，BlurZoom1x区无效，以此类推。\r\n;1 - 5个参数分别为：x1,x2,y1,y2,模糊强度。\r\nBlurNum = 0\r\nBlurZoom01 = 0\r\nBlurZoom02 = 1920\r\nBlurZoom03 = 1040\r\nBlurZoom04 = 1080\r\nBlurZoom05 = 40\r\nBlurZoom11 = 0\r\nBlurZoom12 = 0\r\nBlurZoom13 = 0\r\nBlurZoom14 = 0\r\nBlurZoom15 = 0\r\n\r\n[MaskProcess]\r\n;掩码(颜色识别)模糊\r\n;BlurColor取值-1,0,1，为-1时不进行掩码（颜色识别）模糊，为0时黑色部分进行模糊，为1时，白色部分进行模糊。\r\n;Threshold为阈值，从0~255取值，意为颜色识别的严格程度。为1时候只对纯黑色进行模糊,为254时候只对纯白色进行模糊（阈值过大可能产生边缘毛刺）。\r\nBlurColor = -1\r\nMaskPic = \\mask.png\r\nThreshold = 230\r\nBlurLevel = 120";
		fwrite(msg, sizeof(char), sizeof(msg), stream);
		fclose(stream);
	}
	str = GetPicFile(GetPicUrl());
	while (str == "") {
		Sleep(3000);
		str = GetPicFile(GetPicUrl());
	}
	MakeCache(str);
	SetWallPaper(dir + CacheDir);
	if (argc > 1) {
		char path[MAX_PATH];
		SHGetSpecialFolderPathA(0, path, CSIDL_DESKTOPDIRECTORY, 0);
		std::string SourceFile = dir + CacheDir + "\\" + nDate + ".jpg", DesFile = path;
		DesFile += ("\\" + nDate + ".jpg");
		CopyFileA(SourceFile.c_str(), DesFile.c_str(), FALSE);
	}
	str = dir + "\\config.ini";
	if (GetPrivateProfileIntA("LoginPic", "Statue", 0, str.c_str()) == 1) {
		str = dir + CacheDir + "\\backgroundDefault.jpg";
		SetLoginPaper(str.c_str());
	}
	return 0;
}

