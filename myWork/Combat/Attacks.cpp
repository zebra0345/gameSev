// Combat/Attacks.cpp
#include "Combat/Attacks.h"
#include <fstream>
#include <sstream>

// 내부저장소
// 공격을 키 값으로 실제 공격 데이터를 빠르게 찾을 수 있도록 맵을 사용
static std::unordered_map<std::wstring, AttackDef> gAttacks;

// 공백 제거 유틸
static inline std::wstring Trim(const std::wstring& s)
{
    size_t b = s.find_first_not_of(L" \t\r\n");
    if (b == std::wstring::npos) return L"";
    size_t e = s.find_last_not_of(L" \t\r\n");
    return s.substr(b, e - b + 1);
}

// 파싱함수, 텍스트를 RectF(x, y, w, h) 구조체로 변환
static bool ParseRect4(const std::wstring& v, RectF& out)
{
    float a = 0, b = 0, c = 0, d = 0;
    if (swscanf_s(v.c_str(), L"%f,%f,%f,%f", &a, &b, &c, &d) != 4) return false;
    out.x = a; out.y = b; out.w = c; out.h = d;
    return true;
}

// int 파싱하는 도우미
static bool ParseInt(const std::wstring& v, int& out)
{
    out = _wtoi(v.c_str());
    return true;
}

// float 파싱 도우미
static bool ParseFloat(const std::wstring& v, float& out)
{
    out = (float)_wtof(v.c_str());
    return true;
}

// attacks.ini 파서(설정파일 한줄씩 읽어 공격 데이터 구성)
bool Attacks::LoadFromFile(const std::wstring& path)
{
    gAttacks.clear(); // 혹시 있으면 기존 데이터 비움

    std::wifstream in(path); // 유니코드 파일 열기
    if (!in.is_open())
        return false;

    in.imbue(std::locale("")); // 유니코드 경로/문자 대비(환경 따라 필요)

    std::wstring line;
    AttackDef cur{};
    bool inSection = false;

    // 현재까지 읽은 정보를 맵에 저장, 다음 정보 읽기하는 람다 함수
    auto FlushSection = [&]()
        {
            if (!inSection) return;

            // name이 비어있으면 버림
            if (!cur.name.empty())
                gAttacks[cur.name] = cur;

            cur = AttackDef{};
            inSection = false;
        };

    while (std::getline(in, line)) // 한 줄씩 읽기
    {
        line = Trim(line);
        if (line.empty()) continue;
        if (line[0] == L';') continue; // 주석, 빈줄 무시

        // 섹션 시작: [Attack.Attack1]
        if (line.front() == L'[' && line.back() == L']')
        {
            FlushSection();

            std::wstring sec = line.substr(1, line.size() - 2); // Attack.Attack1
            sec = Trim(sec);

            const std::wstring prefix = L"Attack.";
            if (sec.rfind(prefix, 0) == 0) // Attack.로 시작하면 불러오기
            {
                cur.name = sec.substr(prefix.size()); // Attack1 추출
                inSection = true;
            }
            continue;
        }

        if (!inSection) continue;

        // key=value
        size_t eq = line.find(L'=');
        if (eq == std::wstring::npos) continue;

        std::wstring key = Trim(line.substr(0, eq));
        std::wstring val = Trim(line.substr(eq + 1));

        // ----------- AnimClip 공통 -----------
        if (key == L"Row")            ParseInt(val, cur.clip.row);
        else if (key == L"StartCol")  ParseInt(val, cur.clip.startCol);
        else if (key == L"FrameCount")ParseInt(val, cur.clip.frameCount);
        else if (key == L"FPS")       ParseFloat(val, cur.clip.fps);
        else if (key == L"Loop")
        {
            int tmp = 0; ParseInt(val, tmp);
            cur.clip.loop = (tmp != 0);
        }
        else if (key == L"Inset")     ParseInt(val, cur.clip.inset);
        else if (key == L"ExtendL")   ParseInt(val, cur.clip.extendL);
        else if (key == L"ExtendR")   ParseInt(val, cur.clip.extendR);
        else if (key == L"ExtendT")   ParseInt(val, cur.clip.extendT);
        else if (key == L"ExtendB")   ParseInt(val, cur.clip.extendB);
        else if (key == L"PivotX")    ParseInt(val, cur.clip.pivotX);
        else if (key == L"PivotY")    ParseInt(val, cur.clip.pivotY);

        // ----------- 히트 관련 -----------
        else if (key == L"HitStartFrame") ParseInt(val, cur.clip.hitStartFrame);
        else if (key == L"HitEndFrame")   ParseInt(val, cur.clip.hitEndFrame);
        else if (key == L"HitBox")        ParseRect4(val, cur.clip.hitBoxLocal);

        // ----------- 공격 정의 확장 -----------
        else if (key == L"ChainFrame")    ParseInt(val, cur.chainFrame);
        else if (key == L"Next")          cur.next = val;
        else if (key == L"StageText")     cur.stageText = val;
    }

    FlushSection(); // 마지막 남은 섹션 저장
    return true;
}

// 저장된 데이터에서 이름으로 공격 정보를 검색
const AttackDef* Attacks::Find(const std::wstring& name)
{
    auto it = gAttacks.find(name);
    if (it == gAttacks.end()) return nullptr;
    return &it->second;
}

void Attacks::Clear()
{
    gAttacks.clear();
}

int Attacks::Count()
{
    return (int)gAttacks.size();
}

// 방어적 프로그래밍 설계
// 외부 파일이 없거나 깨졌을 때 대비함
void Attacks::InstallFallbackDefaults()
{
    gAttacks.clear();

    AttackDef a{};
    a.name = L"Attack1";

    // "없어도 죽지 않는" 최소 클립
    a.clip.frameW = 256;
    a.clip.frameH = 256;
    a.clip.row = 0;
    a.clip.startCol = 0;
    a.clip.frameCount = 1;
    a.clip.fps = 1.0f;
    a.clip.loop = false;

    // 히트 프레임/박스도 최소
    a.clip.hitStartFrame = 0;
    a.clip.hitEndFrame = 0;
    a.clip.hitBoxLocal = RectF{ 150.f, 120.f, 90.f, 60.f };

    a.chainFrame = -1;
    a.next = L"";
    a.stageText = L"[Fallback Attack1]";

    gAttacks[a.name] = a;
}