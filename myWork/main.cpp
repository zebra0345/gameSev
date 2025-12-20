// main.cpp
#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>

#include <DirectXMath.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>

#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cwchar>
#include <cstdlib>

#pragma comment(lib, "d3d11.lib")

// 프로젝트 내부 헤더모음
#include "Core/Types.h"
#include "Core/AppTypes.h"
#include "Core/Input.h"

#include "Config/Tuning.h"
#include "Config/Ini.h"

#include "Anim/Animator.h"
#include "Combat/Attacks.h"

#include "Game/Game.h"
#include "Game/GameHooks.h"

#include "UI/FloatingTextSystem.h"

using Microsoft::WRL::ComPtr;

// 전역 상태 시작
// ---- Debug Flags ----
static bool gDebugOverlay = true;
static bool gDebugFreezeAnim = false;
static bool gDebugStepOnce = false;
static bool gDebugBoxes = true;
static bool gPlayerSrcOutOfBounds = false;
static bool gEnemySrcOutOfBounds = false;

// ---- Time / Effects ----
static float gHitStopTimer = 0.0f;
static float gShakeTimer = 0.0f;
static float gTimeAcc = 0.0f;

static float gCamX = 0.0f;
static float gCamY = 0.0f;

// ---- D3D Resources ----(그래픽카드제어)
static ComPtr<ID3D11Device>           gDevice;
static ComPtr<ID3D11DeviceContext>    gContext;
static ComPtr<IDXGISwapChain>         gSwapChain;
static ComPtr<ID3D11RenderTargetView> gRTV;
static D3D11_VIEWPORT                 gViewport{};

static std::unique_ptr<DirectX::SpriteBatch> gSpriteBatch;
static std::unique_ptr<DirectX::SpriteFont>  gFont;

static ComPtr<ID3D11SamplerState>       gPointClampSampler;
static ComPtr<ID3D11ShaderResourceView> gWhiteTex;


// Player 구조체
struct Player
{
    float x = 300.f;
    float y = 420.f;
    float speed = 260.f;

    bool moving = false;
    bool faceRight = true;
    Animator anim; // 애니메이션 상태
    RectF hurtBoxLocal{ 95.f, 120.f, 70.f, 90.f }; //로컬 기본 히트박스

    float inputBufferTimer = 0.f;

    bool attackActive = false;
    RectF attackBoxWorld{}; // 공격 박스 월드좌표

    std::wstring currentAttackName;
    int maxHP = 10;
    int hp = 10;
    bool dead = false;
    float deadTimer = 0.0f;

    float invincibleTimer = 0.0f;
    float hitStunTimer = 0.0f;
    float knockbackVx = 0.0f;
};

// Enemy 상태 및 구조체
enum class EnemyState
{
    // 대기, 추적, 공격준비, 판정, 후딜, 경직, 사망
    Idle, Chase, AttackWindup, AttackActive, AttackRecover, HitStun, Dead
};

struct Enemy
{
    // 기준이 되는 위치들
    float x = 700.f;
    float y = 420.f;
    float moveVx = 0.f;
    float knockVx = 0.f;

    Animator anim;
    bool faceRight = true; // 바라보는 방향

    EnemyState state = EnemyState::Idle;
    float stateTimer = 0.f;
    float invincibleTimer = 0.f;
    
    // 공격쿨타임과 중복 히트 방지
    float attackCooldown = 0.0f;
    bool attackHasHit = false;

    RectF attackBoxWorld{};

    int maxHP = 5;
    int hp = 5;

    bool alive() const { return state != EnemyState::Dead; }
};

// 전역 인스턴스로 선언
static Player gPlayer;
static Enemy  gEnemy;

// 애니메이션 데이터
static AnimClip CLIP_IDLE{ 256,256, 0, 1, 5,  8.0f, true };
static AnimClip CLIP_RUN{ 256,256, 1, 1, 5, 10.0f, true };

static AnimClip E_CLIP_IDLE{ 256,256, 0, 0, 4,  6.0f, true,  0,0,0,0,0, 128,230, -1,-1, RectF{}, 1024,1536, 0,0 };
static AnimClip E_CLIP_RUN{ 256,256, 1, 0, 4, 10.0f, true,  0,0,0,0,0, 128,230, -1,-1, RectF{}, 1024,1536, 0,0 };
static AnimClip E_CLIP_ATK1{ 256,256, 2, 0, 4, 12.0f, false, 0,0,0,0,0, 128,230, -1,-1, RectF{}, 1024,1536, 0,0 };
static AnimClip E_CLIP_ATK2{ 256,256, 3, 0, 4, 12.0f, false, 0,0,0,0,0, 128,230, -1,-1, RectF{}, 1024,1536, 0,0 };
static AnimClip E_CLIP_HIT{ 256,256, 4, 0, 2, 12.0f, false, 0,0,0,0,0, 128,230, -1,-1, RectF{}, 1024,1536, 0,0 };
static AnimClip E_CLIP_DEAD{ 256,256, 5, 0, 3,  8.0f, false, 0,0,0,0,0, 128,230, -1,-1, RectF{}, 1024,1536, 0,0 };


// 카메라 흔들림 업데이트
void UpdateCameraShake(float dtReal)
{
    // 흔들림 시간이 남았다면
    if (gShakeTimer > 0.0f)
    {
        // 시간이 갈수록 세기를 줄어들게, 음수로 내려가면 0으로 보정
        gShakeTimer -= dtReal;
        if (gShakeTimer < 0.0f) gShakeTimer = 0.0f;

        float t = (gTune.shakeSec > 0.0f) ? (gShakeTimer / gTune.shakeSec) : 0.0f;
        float amp = gTune.shakeAmp * t;

        gCamX = sinf(gTimeAcc * 80.0f) * amp;
        gCamY = cosf(gTimeAcc * 95.0f) * amp;
    }
    else
    {
        gCamX = 0.0f;
        gCamY = 0.0f;
    }
}

// 로컬 박스를 월드 박스로 변환
// faceRight가 false일때 좌우반전 오류 해결해서 계산
static RectF LocalBoxToWorld_FrameSpace(const RectF& local, float entityX, float entityY, const AnimClip& clip, bool faceRight)
{   
    // 프레임의 좌상단
    const float frameLeft = entityX - (float)clip.pivotX;
    const float frameTop = entityY - (float)clip.pivotY;

    // 결과 박스는 local을 복사
    RectF w = local;
    // 좌우반전 그대로 더하거나
    if (faceRight)
        w.x = frameLeft + local.x;
    else
        // 피벗기준으로 계산
        w.x = frameLeft + (clip.pivotX * 2.0f - local.x - local.w);
    w.y = frameTop + local.y;
    return w;
}

// 적 몸통 박스를 클립 피벗 기준으로 구성
static RectF EnemyBodyLocalRect(const AnimClip& clip)
{
    // 초기화하고 폭 설정
    RectF r{};
    r.w = gTune.enemyBodyW;
    r.h = gTune.enemyBodyH;
    r.x = (float)clip.pivotX - r.w * 0.5f;
    r.y = (float)clip.pivotY - r.h;
    return r;
}

// 맞는 로직 처리
static void ApplyHitToEnemy(Enemy& e, bool attackerOnLeft)
{
    // 이미 죽었거나 무적이면 처리 x
    if (!e.alive()) return;
    if (e.invincibleTimer > 0.f) return;

    // 데미지, 타격멈춤, 화면흔들림
    e.hp -= 1;
    gHitStopTimer = gTune.hitStopSec;
    gShakeTimer = gTune.shakeSec;

    // 0이하가 되면
    if (e.hp <= 0)
    {
        // 체력 최소값 보정, 상태 사망으로, 이동 제거, 넉백 제거, 사망 애니메이션 설정
        e.hp = 0;
        e.state = EnemyState::Dead;
        e.moveVx = 0.f;
        e.knockVx = 0.f;
        e.anim.SetClip(E_CLIP_DEAD);
        return;
    }

    // 사망이 아니면 피격 경직상태
    // 잠깐 무적 부여, 경직중 이동 정지
    e.state = EnemyState::HitStun;
    e.stateTimer = gTune.enemyHitStunSec;
    e.invincibleTimer = gTune.enemyIFrameSec;
    e.anim.SetClip(E_CLIP_HIT);
    e.moveVx = 0.f;
    e.knockVx = attackerOnLeft ? +gTune.enemyKnockbackVx : -gTune.enemyKnockbackVx;
}

// 플레이어 히트 처리
static void ApplyHitToPlayer(bool attackerOnLeft)
{
    if (gPlayer.hp <= 0 || gPlayer.dead || gPlayer.invincibleTimer > 0.0f) return;

    // 음수 방지
    gPlayer.hp -= 1;
    if (gPlayer.hp < 0) gPlayer.hp = 0;

    // 0이면 사망으로 처리
    if (gPlayer.hp == 0)
    {
        // 사망 플래그, 경과 시간 초기화, 판정 제거 등
        gPlayer.dead = true;
        gPlayer.deadTimer = 0.f;
        gPlayer.currentAttackName.clear();
        gPlayer.attackActive = false;
        gPlayer.inputBufferTimer = 0.f;
        gPlayer.invincibleTimer = 0.f;
        gPlayer.hitStunTimer = 0.f;
        gPlayer.knockbackVx = 0.f;

        // 타격과 흔들림 멈춤
        gHitStopTimer = gTune.hitStopSec;
        gShakeTimer = gTune.shakeSec;

        // 사망 텍스트 표시
        SpawnFloatingText(gPlayer.x - 30.0f, gPlayer.y - 320.0f, L"DEAD");
        return; // 진행하지않고 종료
    }

    // 타격 멈춤, 흔들림, 경직, 무적시간, 넉백 방향
    gHitStopTimer = gTune.hitStopSec;
    gShakeTimer = gTune.shakeSec;
    gPlayer.hitStunTimer = gTune.playerHitStunSec;
    gPlayer.invincibleTimer = gTune.playerIFrameSec;
    gPlayer.knockbackVx = attackerOnLeft ? +gTune.playerKnockbackVx : -gTune.playerKnockbackVx;

    // 텍스트 출력
    SpawnFloatingText(gPlayer.x - 10.0f, gPlayer.y - 320.0f, L"HIT");
}

// 적 리셋
void ResetEnemy()
{
    // 시작위치, 이동속도, 방향, 공격 쿨, 애니메이션 reset
    gEnemy.x = 700.f;
    gEnemy.y = 420.f;
    gEnemy.moveVx = 0.f;
    gEnemy.knockVx = 0.f;
    gEnemy.faceRight = true;
    gEnemy.state = EnemyState::Idle;
    gEnemy.stateTimer = 0.f;
    gEnemy.invincibleTimer = 0.f;
    gEnemy.attackCooldown = 0.f;
    gEnemy.attackHasHit = false;
    gEnemy.maxHP = gTune.enemyMaxHP;
    gEnemy.hp = gTune.enemyMaxHP;
    gEnemy.anim.SetClip(E_CLIP_IDLE);
}

// 플레이어 리셋
void ResetPlayer()
{
    // 시작위치, 체력, 사망해제, 타이머, 무적 등 reset
    gPlayer.x = 300.f;
    gPlayer.y = 420.f;
    gPlayer.maxHP = 10;
    gPlayer.hp = 10;
    gPlayer.dead = false;
    gPlayer.deadTimer = 0.f;
    gPlayer.invincibleTimer = 0.f;
    gPlayer.hitStunTimer = 0.f;
    gPlayer.knockbackVx = 0.f;
    gPlayer.inputBufferTimer = 0.f;
    gPlayer.attackActive = false;
    gPlayer.currentAttackName.clear();
    gPlayer.faceRight = true;
    gPlayer.anim.SetClip(CLIP_IDLE);

    // 텍스트 제거
    ClearFloatingTexts();
}

// 공격중인지 여부 체크
static bool IsAttackPlaying()
{
    // 현재 공격이름이 있고, 루프 애니메이션이 아니고, 아직 애니메이션이 끝나지 않았으면
    return !gPlayer.currentAttackName.empty() && !gPlayer.anim.clip.loop && !gPlayer.anim.finished;
}

// 공격 시작
static bool StartAttack(const std::wstring& name)
{
    // 공격 정의를 이름으로 조회
    const AttackDef* def = Attacks::Find(name);
    // 없으면 실패, 있으면 공격 애니메이션 클립 설정, 이름 기록
    if (!def) return false;
    gPlayer.anim.SetClip(def->clip);
    gPlayer.currentAttackName = name;
    return true; // 시작성공
}

// 공격을 끝내고 이동으로 복귀
static void StopAttackToLocomotion()
{
    // 공격이름 제거, 이동중이면 Run 아니면 Idle
    gPlayer.currentAttackName.clear();
    gPlayer.anim.SetClip(gPlayer.moving ? CLIP_RUN : CLIP_IDLE);
}

// 적 업데이트
void UpdateEnemy(float dtGame)
{
    // 죽은상태시
    if (gEnemy.state == EnemyState::Dead)
    {
        // 이동 정지, 넉백 정지
        gEnemy.moveVx = 0.f;
        gEnemy.knockVx = 0.f;
        // Freeze 아니면 애니메이트
        // 정지상태면 프레임으로 진행
        if (gEnemy.anim.clip.row != E_CLIP_DEAD.row) gEnemy.anim.SetClip(E_CLIP_DEAD);
        if (!gDebugFreezeAnim) gEnemy.anim.Update(dtGame);
        else if (gDebugStepOnce) gEnemy.anim.StepFrame(1);
        return;
    }

    // 플레이어 사망시
    if (gPlayer.dead)
    {
        // 적이 멈추고 게임 종료
        gEnemy.moveVx = 0.f;
        gEnemy.knockVx = 0.f;
        gEnemy.state = EnemyState::Idle;
        gEnemy.anim.SetClip(E_CLIP_IDLE);
        return;
    }

    // 적 무적  타이머
    if (gEnemy.invincibleTimer > 0.f)
    {
        // 시간 빼면서 0 아래로 내려가지 않도록 보정
        gEnemy.invincibleTimer -= dtGame;
        if (gEnemy.invincibleTimer < 0.f) gEnemy.invincibleTimer = 0.f;
    }
    
    // 적 공격 쿨타임 처리
    if (gEnemy.attackCooldown > 0.f)
    {
        // 이 역시 음수 안내려가게 보정
        gEnemy.attackCooldown -= dtGame;
        if (gEnemy.attackCooldown < 0.f) gEnemy.attackCooldown = 0.f;
    }

    // 플레이어와 적의 거리, 절대 거리, 오른쪽 체크
    float dx = gPlayer.x - gEnemy.x;
    float absDx = fabsf(dx);
    gEnemy.faceRight = (dx >= 0.f);

    // 현재 적 상태에 따라 행동 분기시킴
    switch (gEnemy.state)
    {
    // 기본은 정지
    case EnemyState::Idle:
        gEnemy.moveVx = 0.f;
        // 일정거리로 들어오면 추적으로 전환
        if (gEnemy.anim.clip.row != E_CLIP_IDLE.row) gEnemy.anim.SetClip(E_CLIP_IDLE);
        if (absDx <= gTune.enemyAggroRange)
        {
            gEnemy.state = EnemyState::Chase;
            gEnemy.anim.SetClip(E_CLIP_RUN);
        }
        break;
    // 추적
    case EnemyState::Chase:
        // 너무 가까우면 멈춤
        if (gEnemy.anim.clip.row != E_CLIP_RUN.row) gEnemy.anim.SetClip(E_CLIP_RUN);
        if (absDx <= gTune.enemyStopRange) gEnemy.moveVx = 0.f;
        else gEnemy.moveVx = ((dx > 0.f) ? 1.f : -1.f) * gTune.enemyChaseSpeed;

        // 공격 범위에 있으면
        if (absDx <= gTune.enemyAttackRange && gEnemy.attackCooldown <= 0.f)
        {
            // 공격 준비, 이동 금지, 공격모션 랜덤 선택(지금은 없음 모션이)
            gEnemy.state = EnemyState::AttackWindup;
            gEnemy.stateTimer = gTune.enemyAttackWindupSec;
            gEnemy.moveVx = 0.f;
            gEnemy.attackHasHit = false;
            int r = rand() % 2;
            gEnemy.anim.SetClip(r == 0 ? E_CLIP_ATK1 : E_CLIP_ATK2);
        }

        // 너무 멀어지면 대기로 복귀
        if (absDx > gTune.enemyAggroRange * 1.2f)
        {
            gEnemy.state = EnemyState::Idle;
            gEnemy.moveVx = 0.f;
            gEnemy.anim.SetClip(E_CLIP_IDLE);
        }
        break;

    // 공격 준비
    case EnemyState::AttackWindup:
        // 준비중 이동 정지, 시간 감소
        gEnemy.moveVx = 0.f;
        gEnemy.stateTimer -= dtGame;
        // 공격 준비 끝나면 활성으로 전환
        if (gEnemy.stateTimer <= 0.f)
        {
            gEnemy.state = EnemyState::AttackActive;
            gEnemy.stateTimer = gTune.enemyAttackActiveSec;
            gEnemy.attackHasHit = false;
        }
        break;

    // 활성화해서 맞았는지 체크
    case EnemyState::AttackActive:
        // 이동 정지 후 히트 처리
        gEnemy.moveVx = 0.f;
        gEnemy.attackBoxWorld = LocalBoxToWorld_FrameSpace(gTune.enemyAttackBoxLocal, gEnemy.x, gEnemy.y, gEnemy.anim.clip, gEnemy.faceRight);
        if (!gEnemy.attackHasHit)
        {
            // 데미지 적용과 플레이어 히트박스 계산
            RectF playerHurt = LocalBoxToWorld_FrameSpace(gPlayer.hurtBoxLocal, gPlayer.x, gPlayer.y, gPlayer.anim.clip, gPlayer.faceRight);
            if (Intersect(gEnemy.attackBoxWorld, playerHurt))
            {
                ApplyHitToPlayer(gEnemy.x < gPlayer.x);
                gEnemy.attackHasHit = true;
            }
        }
        gEnemy.stateTimer -= dtGame;
        if (gEnemy.stateTimer <= 0.f)
        {
            gEnemy.state = EnemyState::AttackRecover;
            gEnemy.stateTimer = gTune.enemyAttackRecoverSec;
            gEnemy.attackCooldown = gTune.enemyAttackCooldownSec;
        }
        break;

    // 후딜레이 처리
    case EnemyState::AttackRecover:
        // 상태를 원상복구하는 코드
        gEnemy.moveVx = 0.f;
        gEnemy.stateTimer -= dtGame;
        if (gEnemy.stateTimer <= 0.f)
        {
            gEnemy.state = EnemyState::Chase;
            gEnemy.anim.SetClip(E_CLIP_RUN);
        }
        break;

    // hit경직시 이동 정지
    case EnemyState::HitStun:
        // 경직 끝나면 다시 추적
        // 경직이 걸렸다 -> 플레이어가 근처에 있다로 간주하여 추적으로 전환
        gEnemy.moveVx = 0.f;
        if (gEnemy.anim.clip.row != E_CLIP_HIT.row) gEnemy.anim.SetClip(E_CLIP_HIT);
        gEnemy.stateTimer -= dtGame;
        if (gEnemy.stateTimer <= 0.f)
        {
            gEnemy.state = EnemyState::Chase;
            gEnemy.anim.SetClip(E_CLIP_RUN);
        }
        break;
    }

    // AI이동과 넉백을 합친 실제 값
    float totalVx = gEnemy.moveVx + gEnemy.knockVx;
    if (totalVx != 0.f) gEnemy.x += totalVx * dtGame;

    // 넉백이 남아있으면?
    if (fabsf(gEnemy.knockVx) > 0.f)
    {   
        // 감쇠량 계산하여 처리
        // 양수 음수 넉백 줄이기
        float damp = gTune.enemyKnockbackDamp * dtGame;
        if (gEnemy.knockVx > 0.f) { gEnemy.knockVx -= damp; if (gEnemy.knockVx < 0.f) gEnemy.knockVx = 0.f; }
        else { gEnemy.knockVx += damp; if (gEnemy.knockVx > 0.f) gEnemy.knockVx = 0.f; }
    }

    // Freeze디버그 아니면 애니메이션 업데이트
    if (!gDebugFreezeAnim) gEnemy.anim.Update(dtGame);
    else if (gDebugStepOnce) gEnemy.anim.StepFrame(1);
}

// 플레이어 업데이트
// dtgame은 게임 로직 기준 델타 타임, 초
void UpdatePlayer(float dtGame)
{
    // 이번 프레임 이동 여부
    gPlayer.moving = false;
    // 죽었으면 시간만 흐름
    if (gPlayer.dead) { gPlayer.deadTimer += dtGame; return; }

    // 무적시간이 남아있으면
    if (gPlayer.invincibleTimer > 0.f)
    {
        // 감소시켜야함
        gPlayer.invincibleTimer -= dtGame;
        if (gPlayer.invincibleTimer < 0.f) gPlayer.invincibleTimer = 0.f;
    }
    // 피격 상태인지 경직 상태인지 계산
    bool inHitStun = (gPlayer.hitStunTimer > 0.f);
    // 경직상태면?
    if (inHitStun)
    {
        // 시간 감소 및 0 아래 보정
        gPlayer.hitStunTimer -= dtGame;
        if (gPlayer.hitStunTimer < 0.f) gPlayer.hitStunTimer = 0.f;
    }
    
    // 입력 버퍼 체크
    if (gPlayer.inputBufferTimer > 0.f)
    {
        gPlayer.inputBufferTimer -= dtGame;
        if (gPlayer.inputBufferTimer < 0.f) gPlayer.inputBufferTimer = 0.f;
    }

    // 넉백 속도가 남아있다면
    if (fabsf(gPlayer.knockbackVx) > 0.0f)
    {
        // 위치로 이동, 서서히 넉백을 줄임
        gPlayer.x += gPlayer.knockbackVx * dtGame;
        float damp = gTune.playerKnockbackDamp * dtGame;
        if (gPlayer.knockbackVx > 0.f) { gPlayer.knockbackVx -= damp; if (gPlayer.knockbackVx < 0.f) gPlayer.knockbackVx = 0.f; }
        else { gPlayer.knockbackVx += damp; if (gPlayer.knockbackVx > 0.f) gPlayer.knockbackVx = 0.f; }
    }

    // Input & Move
    bool lockMove = inHitStun || (gTune.lockMoveDuringAttack && IsAttackPlaying());
    if (!lockMove) // 이동이 안잠겨있으면
    {
        // A 또는 왼쪽방향키 이동
        if (IsDown('A') || IsDown(VK_LEFT))
        {
            gPlayer.x -= gPlayer.speed * dtGame;
            gPlayer.moving = true;
            gPlayer.faceRight = false;
        }
        // D또는 오른쪽방향키 이동
        if (IsDown('D') || IsDown(VK_RIGHT))
        {
            gPlayer.x += gPlayer.speed * dtGame;
            gPlayer.moving = true;
            gPlayer.faceRight = true;
        }
    }

    // 경직이 아니고 스페이스 누르면
    if (!inHitStun && IsPressedOnce(VK_SPACE))
    {
        // 이미 공격중일시 콤보연결
        if (IsAttackPlaying())
        {
            gPlayer.inputBufferTimer = gTune.inputBufferSec;
        }
        else
        {
            if (StartAttack(L"Attack1")) // 아닐시 첫공격 시작
            {
                const AttackDef* a = Attacks::Find(L"Attack1");
                if (a) SpawnFloatingText(gPlayer.x - 20.0f, gPlayer.y - 280.0f, a->stageText);
            }
        }
    }

    // 디버그 프리즈 모드 체크(1프레임씩 진행)
    if (!gDebugFreezeAnim) gPlayer.anim.Update(dtGame);
    else if (gDebugStepOnce) gPlayer.anim.StepFrame(1);

    // 공격중일시 콤보 연결 검사
    if (IsAttackPlaying())
    {
        // 현재 공격 정의를 찾고
        const AttackDef* cur = Attacks::Find(gPlayer.currentAttackName);
        if (cur)
        {
            // 입력 버퍼가 남아 있고 체인 설정시
            if (gPlayer.inputBufferTimer > 0.f && cur->chainFrame >= 0)
            {
                if (gPlayer.anim.currentFrame >= cur->chainFrame && !cur->next.empty())
                {
                    if (StartAttack(cur->next)) // 다음 공격 시작
                    {
                        // 입력 버퍼 소모
                        gPlayer.inputBufferTimer = 0.f;
                        const AttackDef* nextAtk = Attacks::Find(cur->next);
                        if (nextAtk) SpawnFloatingText(gPlayer.x - 20.0f, gPlayer.y - 280.0f, nextAtk->stageText);
                    }
                }
            }
        }
    }

    // 루프 애니가 아니고 끝났으면
    if (!gPlayer.anim.clip.loop && gPlayer.anim.finished)
    {
        // 입력 버퍼 정리, 이동 상태 복귀
        gPlayer.inputBufferTimer = 0.f;
        StopAttackToLocomotion();
    }
    else
    {
        // 루프 애니 상태라면
        if (gPlayer.anim.clip.loop)
        {
            // 이동중이면 Run 아니면 Idle스프라이트 처리
            if (gPlayer.moving) { if (gPlayer.anim.clip.row != CLIP_RUN.row) gPlayer.anim.SetClip(CLIP_RUN); }
            else { if (gPlayer.anim.clip.row != CLIP_IDLE.row) gPlayer.anim.SetClip(CLIP_IDLE); }
        }
    }

    // 공격 판정은 매 프레임 기본 false
    gPlayer.attackActive = false;
    // 현재 애니메이션 참조
    const AnimClip& c = gPlayer.anim.clip;
    // 히트 프레임 구간이 정상이면
    if (!c.loop && c.hitStartFrame >= 0 && c.hitEndFrame >= c.hitStartFrame)
    {
        // 프레임 번호와 히트 구간인지 체크
        int f = gPlayer.anim.currentFrame;
        if (c.hitStartFrame <= f && f <= c.hitEndFrame)
        {
            // 공격 판정 활성화
            gPlayer.attackActive = true;
            gPlayer.attackBoxWorld = LocalBoxToWorld_FrameSpace(c.hitBoxLocal, gPlayer.x, gPlayer.y, c, gPlayer.faceRight);
        }
    }

    // 적이 살아있고 공격판정이 켜지면
    if (gEnemy.alive() && gPlayer.attackActive)
    {
        RectF enemyBody = LocalBoxToWorld_FrameSpace(EnemyBodyLocalRect(gEnemy.anim.clip), gEnemy.x, gEnemy.y, gEnemy.anim.clip, gEnemy.faceRight);
        if (Intersect(gPlayer.attackBoxWorld, enemyBody))
        {
            ApplyHitToEnemy(gEnemy, gPlayer.x < gEnemy.x);
        }
    }
}

// 랜더링 파트
// 1. 화면을 일단 지움
// 2. 오브젝트 그림
// 3. UI그림

// Float검사
static bool IsFiniteFloat(float v)
{
    return std::isfinite(v) != 0;
}

// RectF가 정상 범위인지 검사
static bool IsSaneRect(const RectF& r)
{
    if (!IsFiniteFloat(r.x) || !IsFiniteFloat(r.y) || !IsFiniteFloat(r.w) || !IsFiniteFloat(r.h))
        return false;

    if (r.w <= 0.0f || r.h <= 0.0f)
        return false;

    const float kMax = 100000.0f;
    if (fabsf(r.x) > kMax || fabsf(r.y) > kMax || r.w > kMax || r.h > kMax)
        return false;

    return true;
}

// 사각형 채우기
static void DrawFilledRect(const RectF& r, const DirectX::XMVECTORF32& color)
{
    // 텍스처나 배치가 없으면 그릴 수 없음
    if (!gWhiteTex || !gSpriteBatch) return;

    if (!IsSaneRect(r))
    {
        OutputDebugStringW(L"[Overlay] Skip insane rect (NaN/INF/huge)\n");
        return;
    }

    // 흰색 텍스처를 컬러로 칠해 사각형처럼 그림
    gSpriteBatch->Draw(gWhiteTex.Get(), ToRECT(r), nullptr, color);
}

// 테두리 사각형 그리기
static void DrawRectOutline(const RectF& r, float t, const DirectX::XMVECTORF32& color)
{
    DrawFilledRect(RectF{ r.x, r.y, r.w, t }, color);
    DrawFilledRect(RectF{ r.x, r.y + r.h - t, r.w, t }, color);
    DrawFilledRect(RectF{ r.x, r.y, t, r.h }, color);
    DrawFilledRect(RectF{ r.x + r.w - t, r.y, t, r.h }, color);
}

// 십자 표시 - 캐릭터의 발쪽에 십자표시함
static void DrawCross(float x, float y, float size, float t, const DirectX::XMVECTORF32& color)
{
    DrawFilledRect(RectF{ x - size, y - t * 0.5f, size * 2.0f, t }, color);
    DrawFilledRect(RectF{ x - t * 0.5f, y - size, t, size * 2.0f }, color);
}

// 발 좌표 기준 바디 사각형 생성
static RectF MakePrimitiveBodyRect(float footX, float footY, float w, float h)
{
    RectF r; r.w = w; r.h = h; r.x = (footX - w * 0.5f); r.y = (footY - h); return r;
}

// 플레이어 상태별 색상 결정
static DirectX::XMVECTORF32 ColorForPlayer()
{
    // 죽었으면 어둡게, 맞는중이면 빨갛게, 공격중이면 노랑, 이동중이면 밝은 파랑
    if (gPlayer.dead) return DirectX::XMVECTORF32{ 0.2f,0.2f,0.2f,1.0f };
    if (gPlayer.hitStunTimer > 0.f) return DirectX::XMVECTORF32{ 1.f,0.4f,0.4f,1.0f };
    if (IsAttackPlaying()) return DirectX::XMVECTORF32{ 1.f,0.9f,0.2f,1.0f };
    if (gPlayer.moving) return DirectX::XMVECTORF32{ 0.6f,0.9f,1.0f,1.0f };
    return DirectX::XMVECTORF32{ 0.7f,0.8f,1.0f,1.0f };
}

// 적 상태별 색상 결정
static DirectX::XMVECTORF32 ColorForEnemy()
{
    // 죽었으면 어둡게, 피격중이면 빨갛게, 공격중이면 분홍
    if (gEnemy.state == EnemyState::Dead) return DirectX::XMVECTORF32{ 0.2f,0.2f,0.2f,1.0f };
    if (gEnemy.state == EnemyState::HitStun) return DirectX::XMVECTORF32{ 1.f,0.4f,0.4f,1.0f };
    if (gEnemy.state == EnemyState::AttackActive) return DirectX::XMVECTORF32{ 1.f,0.2f,0.9f,1.0f };
    return DirectX::XMVECTORF32{ 0.9f,0.7f,0.6f,1.0f };
}

// 플레이어 HP UI 출력
static void RenderPlayerHP()
{
    // 텍스쳐 없으면 종료
    if (!gWhiteTex) return;
    // 배경 바 위치 크기
    // 비율만큼 폭 조정
    float ratio = (gPlayer.maxHP > 0) ? (float)gPlayer.hp / (float)gPlayer.maxHP : 0.f;
    RectF bg{ 20.f, 20.f, 240.f, 18.f };
    RectF fg = bg; fg.w = bg.w * ratio;
    gSpriteBatch->Draw(gWhiteTex.Get(), ToRECT(bg), nullptr, DirectX::XMVECTORF32{ 0.f,0.f,0.f,0.7f });
    gSpriteBatch->Draw(gWhiteTex.Get(), ToRECT(fg), nullptr, DirectX::XMVECTORF32{ 1.f,0.f,0.f,0.9f });
    if (gFont) {
        // 폰트 로드
        std::wstring s = L"HP " + std::to_wstring(gPlayer.hp) + L"/" + std::to_wstring(gPlayer.maxHP);
        gFont->DrawString(gSpriteBatch.get(), s.c_str(), DirectX::XMFLOAT2(24.f, 44.f));
    }
}

// 적 HP 바 출력, 카메라 오프셋을 반영
static void RenderEnemyHPBar(float camX, float camY)
{
    // 텍스처가 없거나 적이 죽으면 생략
    if (!gWhiteTex || gEnemy.state == EnemyState::Dead) return;
    float hpRatio = (gEnemy.maxHP > 0) ? (float)gEnemy.hp / (float)gEnemy.maxHP : 0.f;
    RectF bodyW = LocalBoxToWorld_FrameSpace(EnemyBodyLocalRect(gEnemy.anim.clip), gEnemy.x, gEnemy.y, gEnemy.anim.clip, gEnemy.faceRight);
    float centerX = bodyW.x + bodyW.w * 0.5f;
    float topY = bodyW.y;

    // HP바 배경, 폭, 높이 등 설정
    RectF barBG; barBG.w = gTune.hpBarW; barBG.h = gTune.hpBarH;
    barBG.x = (centerX - barBG.w * 0.5f) + camX;
    barBG.y = (topY - gTune.hpBarOffsetY) + camY;
    RectF barFG = barBG; barFG.w = barBG.w * hpRatio;

    gSpriteBatch->Draw(gWhiteTex.Get(), ToRECT(barBG), nullptr, DirectX::XMVECTORF32{ 0.f,0.f,0.f,0.7f });
    gSpriteBatch->Draw(gWhiteTex.Get(), ToRECT(barFG), nullptr, DirectX::XMVECTORF32{ 1.f,0.f,0.f,0.9f });
}

// 외부에서 호출하는 간단한 래퍼
void RenderEnemyHPBar()
{
    RenderEnemyHPBar(gCamX, gCamY);
}

// 디버그용 오버레이를 그리는 함수
static void DrawDebugOverlay()
{
    // 오버레이가 꺼져있으면 안그림
    if (!gDebugOverlay || !gSpriteBatch || !gFont) return;
    {
        // 한번만 검사하고 값 저장을 깔끔하게 블록화
        // 소스 사각형이 시트 밖인지, 적 애니메이션이 동일하게 OOB여부 확인
        bool oobP = false, oobE = false;
        (void)gPlayer.anim.GetSourceRectUnchecked(&oobP);
        (void)gEnemy.anim.GetSourceRectUnchecked(&oobE);
        gPlayerSrcOutOfBounds = oobP; gEnemySrcOutOfBounds = oobE;
    }
    // 플레이어 기준점
    DrawCross(gPlayer.x + gCamX, gPlayer.y + gCamY, 10.0f, 2.0f, DirectX::Colors::Yellow);

    // 플레이어 피격 박스를 월드 좌표로 변환, 피격박스, 위치, 클립, 방향
    RectF playerHurt = LocalBoxToWorld_FrameSpace(gPlayer.hurtBoxLocal, gPlayer.x, gPlayer.y, gPlayer.anim.clip, gPlayer.faceRight);
    
    // 카메라 흔들림 오프셋을 실제 화면 좌표에 반영
    playerHurt.x += gCamX; playerHurt.y += gCamY;
    DrawRectOutline(playerHurt, 2.0f, DirectX::Colors::Lime);

    // 플레이어 공격판정이 켜져있으면
    if (gPlayer.attackActive) {
        // 이미 계산된 월드 공격 박스를 복사
        RectF atk = gPlayer.attackBoxWorld; atk.x += gCamX; atk.y += gCamY;
        DrawRectOutline(atk, 2.0f, DirectX::Colors::Red);
    }

    // 적 몸통박스를 월드좌표 기준으로 그림
    RectF enemyBody = LocalBoxToWorld_FrameSpace(EnemyBodyLocalRect(gEnemy.anim.clip), gEnemy.x, gEnemy.y, gEnemy.anim.clip, gEnemy.faceRight);
    enemyBody.x += gCamX; enemyBody.y += gCamY; // 카메라 오프셋 반영함
    DrawRectOutline(enemyBody, 2.0f, DirectX::Colors::Cyan);

    if (gEnemy.state == EnemyState::AttackActive) {
        RectF eAtk = gEnemy.attackBoxWorld; eAtk.x += gCamX; eAtk.y += gCamY;
        DrawRectOutline(eAtk, 2.0f, DirectX::Colors::Orange);
    }

    // 화면에 출력할 텍스트
    wchar_t buf[512]{};
    // 플레이어 현재 클립을 참조로 꺼내서 짧게 쓰기 위함
    const AnimClip& pc = gPlayer.anim.clip;

    // 플레이어 디버그 정보를 문자열로 조립
    // 담을 버퍼
    swprintf_s(buf, L"[Player] row=%d frame=%d/%d hit=%d~%d atk=%d freeze=%d",
        pc.row, gPlayer.anim.currentFrame, pc.frameCount, pc.hitStartFrame, pc.hitEndFrame, (int)gPlayer.attackActive, (int)gDebugFreezeAnim);
    
    // 위에서 만든 디버그 문자열 화면에 출력
    gFont->DrawString(gSpriteBatch.get(), buf, DirectX::XMFLOAT2(10.0f, 10.0f), DirectX::Colors::White);

    // 적 현재 클립 참조
    const AnimClip& ec = gEnemy.anim.clip;
    swprintf_s(buf, L"[Enemy ] state=%d row=%d frame=%d/%d inv=%.2f",
        (int)gEnemy.state, ec.row, gEnemy.anim.currentFrame, ec.frameCount, gEnemy.invincibleTimer);
    gFont->DrawString(gSpriteBatch.get(), buf, DirectX::XMFLOAT2(10.0f, 32.0f), DirectX::Colors::White);

    // 시트범위 바깥 체크
    if (gPlayerSrcOutOfBounds) gFont->DrawString(gSpriteBatch.get(), L"[WARN] Player SrcRect OOB", DirectX::XMFLOAT2(10.0f, 54.0f), DirectX::Colors::Red);
    if (gEnemySrcOutOfBounds) gFont->DrawString(gSpriteBatch.get(), L"[WARN] Enemy SrcRect OOB", DirectX::XMFLOAT2(10.0f, 74.0f), DirectX::Colors::Red);
}

// Hooks에 연결
// 한 프레임을 그리는 함수. Game.Tick에서 매 프레임 호출됨
void RenderFrame() 
{
    // 화면을 그리는 기본설정
    float clearColor[4] = { 0.f, 0.f, 0.f, 1.f };
    gContext->OMSetRenderTargets(1, gRTV.GetAddressOf(), nullptr);
    gContext->RSSetViewports(1, &gViewport);
    gContext->ClearRenderTargetView(gRTV.Get(), clearColor);

    // 랜더링 시작
    gSpriteBatch->Begin(DirectX::SpriteSortMode_Deferred, nullptr, gPointClampSampler.Get());

    // 오류시 디버깅용 출력
    if (!gWhiteTex) OutputDebugStringW(L"[Render] gWhiteTex is null\n");
    if (!gSpriteBatch) OutputDebugStringW(L"[Render] gSpriteBatch is null\n");
    if (!gFont) OutputDebugStringW(L"[Render] gFont is null\n");

    // 1. Player Draw
    if (gWhiteTex) {
        // 흰 텍스쳐가 있을때
        bool blinkOff = (gPlayer.invincibleTimer > 0.f && fmodf(gPlayer.invincibleTimer, 0.10f) < 0.05f);
        if (!blinkOff) {
            // 무적시간동안 깜빡이게
            // 깜빡임으로 이번 구간이 off가 아니라면 플레이어 몸통 그리기
            RectF body = MakePrimitiveBodyRect(gPlayer.x + gCamX, gPlayer.y + gCamY, 80.f, 140.f);
            gSpriteBatch->Draw(gWhiteTex.Get(), ToRECT(body), nullptr, ColorForPlayer());
            if (gPlayer.attackActive) {
                RectF atk = gPlayer.attackBoxWorld; atk.x += gCamX; atk.y += gCamY;
                gSpriteBatch->Draw(gWhiteTex.Get(), ToRECT(atk), nullptr, DirectX::XMVECTORF32{ 1.f, 0.2f, 0.2f, 0.7f });
            }
        }
    }

    // 2. Enemy Draw
    // 적도 동일한 흰 텍스쳐
    if (gWhiteTex) {
        // 몬스터도 플레이어와 동일한 로직
        bool blinkOff = (gEnemy.invincibleTimer > 0.f && fmodf(gEnemy.invincibleTimer, 0.10f) < 0.05f);
        if (!blinkOff) {
            RectF bodyW = LocalBoxToWorld_FrameSpace(EnemyBodyLocalRect(gEnemy.anim.clip), gEnemy.x, gEnemy.y, gEnemy.anim.clip, gEnemy.faceRight);
            bodyW.x += gCamX; bodyW.y += gCamY;
            gSpriteBatch->Draw(gWhiteTex.Get(), ToRECT(bodyW), nullptr, ColorForEnemy());
        }

        // 디버그 박스 그리기
        if (gDebugBoxes && gEnemy.alive()) {
            RectF b = LocalBoxToWorld_FrameSpace(EnemyBodyLocalRect(gEnemy.anim.clip), gEnemy.x, gEnemy.y, gEnemy.anim.clip, gEnemy.faceRight);
            b.x += gCamX; b.y += gCamY; // 카메라 오프셋
            gSpriteBatch->Draw(gWhiteTex.Get(), ToRECT(b), nullptr, DirectX::XMVECTORF32{ 0.f,1.f,0.f,0.18f });
        }
        if (gDebugBoxes && gPlayer.attackActive) {
            RectF atk = gPlayer.attackBoxWorld; atk.x += gCamX; atk.y += gCamY;
            gSpriteBatch->Draw(gWhiteTex.Get(), ToRECT(atk), nullptr, DirectX::XMVECTORF32{ 1.f,1.f,0.f,0.30f });
        }
        if (gDebugBoxes && gEnemy.state == EnemyState::AttackActive) {
            RectF eAtk = gEnemy.attackBoxWorld; eAtk.x += gCamX; eAtk.y += gCamY;
            gSpriteBatch->Draw(gWhiteTex.Get(), ToRECT(eAtk), nullptr, DirectX::XMVECTORF32{ 0.f,0.7f,1.f,0.30f });
        }
    }

    // 3. UI
    RenderEnemyHPBar(gCamX, gCamY);
    RenderPlayerHP();
    DrawDebugOverlay();

    // FloatingText는 GetFloatingTexts()로만 읽음(중복저장 해결)
    if (gFont) {
        // 폰트 있을시
        // 떠다니는 텍스트 출력
        const auto& texts = GetFloatingTexts();
        for (const auto& t : texts) {
            gFont->DrawString(gSpriteBatch.get(), t.text.c_str(), DirectX::XMFLOAT2(t.x + gCamX, t.y + gCamY));
        }

        // 현재 랜더링, 디버깅 문자열 표시
        std::wstring mode = (gRenderMode == RenderMode::Primitive) ? L"[F1] Primitive" : L"[F1] Sprite (Disabled)";
        std::wstring dbg = (gDebugBoxes ? L"[F6] DebugBoxes ON" : L"[F6] DebugBoxes OFF");
        gFont->DrawString(gSpriteBatch.get(), mode.c_str(), DirectX::XMFLOAT2(20.f, 660.f));
        gFont->DrawString(gSpriteBatch.get(), dbg.c_str(), DirectX::XMFLOAT2(20.f, 688.f));
        gFont->DrawString(gSpriteBatch.get(), L"[F3] Freeze  [F4] Step  [F5] Overlay", DirectX::XMFLOAT2(20.f, 716.f));

        if (gPlayer.dead) {
            gFont->DrawString(gSpriteBatch.get(), L"GAME OVER", DirectX::XMFLOAT2(520.f, 300.f));
            gFont->DrawString(gSpriteBatch.get(), L"Press R to Restart", DirectX::XMFLOAT2(480.f, 340.f));
        }
    }

    gSpriteBatch->End();
    gSwapChain->Present(1, 0);
}

//  // DirectX 11 초기화. 성공하면 true 반환
static bool InitD3D(HWND hWnd)
{
    // 스왑체인 구조체, 더블버퍼링, 화면픽셀
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    // 실제 만들어진 디바이스 기능 받음
    D3D_FEATURE_LEVEL fl{};
    // 기본 어댑터 사용해서 한번에 생성
    // 포인터들을 넣고 컨텍스트를 한번에 생성
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
        &sd, gSwapChain.GetAddressOf(), gDevice.GetAddressOf(), &fl, gContext.GetAddressOf());
    // 생성실패 -> 오류(종료)
    if (FAILED(hr)) return false;

    // 백버퍼 텍스쳐
    ComPtr<ID3D11Texture2D> backBuffer;
    gSwapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
    gDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, gRTV.GetAddressOf());

    // 랜더링 크기설정
    gViewport.Width = 1280.f; gViewport.Height = 720.f; gViewport.MaxDepth = 1.f;
    gContext->RSSetViewports(1, &gViewport);

    gSpriteBatch = std::make_unique<DirectX::SpriteBatch>(gContext.Get());


    D3D11_SAMPLER_DESC samp{};
    samp.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    gDevice->CreateSamplerState(&samp, gPointClampSampler.GetAddressOf());

    // 1x1 흰색 네모 텍스쳐 설정
    const UINT32 white = 0xFFFFFFFF;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = 1; desc.Height = 1; desc.MipLevels = 1; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_IMMUTABLE; desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA init{}; init.pSysMem = &white; init.SysMemPitch = sizeof(UINT32);
    ComPtr<ID3D11Texture2D> tex;
    gDevice->CreateTexture2D(&desc, &init, tex.GetAddressOf());
    gDevice->CreateShaderResourceView(tex.Get(), nullptr, gWhiteTex.GetAddressOf());

    // 폰트 로드
    std::wstring fontPath = GetExeDir() + L"\\assets\\DebugFont.spritefont";
    if (GetFileAttributesW(fontPath.c_str()) != INVALID_FILE_ATTRIBUTES)
        gFont = std::make_unique<DirectX::SpriteFont>(gDevice.Get(), fontPath.c_str());

    // Ini로 불러오기
    LoadTuningFromIni();
    {
        std::wstring atkIni = GetExeDir() + L"\\assets\\attacks.ini";
        if (!Attacks::LoadFromFile(atkIni))
        {
            OutputDebugStringW(L"[Init] attacks.ini load failed -> InstallFallbackDefaults\n");
            Attacks::InstallFallbackDefaults();
        }
    }

    // 상태 초기화
    ResetPlayer();
    ResetEnemy();
    return true;
}

// DirectX 리소스를 정리하는 함수
// 폰트, 흰색네모 등 정리
static void ShutdownD3D()
{
    gFont.reset(); gSpriteBatch.reset();
    gWhiteTex.Reset(); gPointClampSampler.Reset();
    gRTV.Reset(); gSwapChain.Reset(); gContext.Reset(); gDevice.Reset();
}

// 윈도우 메시지 처리 함수
// 사용자 종료시 반응 처리
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    if (msg == WM_KEYDOWN && wParam == VK_ESCAPE) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// 외부에서 사망여부 체크가능하게[
static bool IsPlayerDead()
{
    return gPlayer.dead;
}

// 프로그램 진입점
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
{
    // 창 클래스 기본 설정
    WNDCLASS wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"DNFClientProto";
    RegisterClass(&wc);

    // 창 제목과 기본 설정 세팅
    HWND hWnd = CreateWindowEx(0, wc.lpszClassName, L"DNF Content Client Prototype", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, nullptr, nullptr, hInst, nullptr);
    if (!hWnd) return -1;
    ShowWindow(hWnd, nCmdShow);

    if (!InitD3D(hWnd)) return -1;

    // 게임 오케스트레이션
    Game game;
    game.Initialize();

    GameHooks hooks{};

    // State Binding(랜더, 디버그, 시간, 적 등)
    hooks.renderMode = &gRenderMode;
    hooks.debugFreezeAnim = &gDebugFreezeAnim;
    hooks.debugStepOnce = &gDebugStepOnce;
    hooks.debugOverlay = &gDebugOverlay;
    hooks.debugBoxes = &gDebugBoxes;
    hooks.hitStopTimer = &gHitStopTimer;
    hooks.shakeTimer = &gShakeTimer;
    hooks.timeAcc = &gTimeAcc;
    hooks.player = &gPlayer;
    hooks.enemy = &gEnemy;

    // GameHooks가 std::vector<FloatingText>*를 요구해서, const_cast로 포인터만 제공(직접 push/erase 금지)
    {
        auto& texts = const_cast<std::vector<FloatingText>&>(GetFloatingTexts());
        hooks.floatingTexts = &texts;
    }

    hooks.isPlayerDead = &IsPlayerDead;

    // Function Binding
    // 눌림체크, 업데이트 포인터 등 모음
    hooks.isPressedOnce = &IsPressedOnce;

    hooks.updateCameraShake = &UpdateCameraShake;
    hooks.updateFloatingTexts = &UpdateFloatingTexts; // UI/FloatingTextSystem.cpp 구현 사용
    hooks.updatePlayer = &UpdatePlayer;
    hooks.updateEnemy = &UpdateEnemy;
    hooks.renderFrame = &RenderFrame;
    hooks.resetPlayer = &ResetPlayer;
    hooks.resetEnemy = &ResetEnemy;

    game.Bind(hooks);

    // Main Loop
    MSG msg{};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            game.Tick();
        }
    }

    ShutdownD3D();
    return (int)msg.wParam;
}
