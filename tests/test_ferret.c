#include "jpb/ferret.h"

#include "jpb/effects.h"
#include "jpb/whook.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

enum {
    VK_PRIOR_TEST = 0x21,
    VK_NEXT_TEST = 0x22,
    VK_LEFT_TEST = 0x25,
    VK_UP_TEST = 0x26,
    VK_RIGHT_TEST = 0x27,
    VK_DOWN_TEST = 0x28
};

static void press_key(int virtual_key)
{
    jpb_WHookClearKeyState();
    jpb_WHookHandleKeyEvent(virtual_key, 0, 1);
}

static void release_keys(void)
{
    jpb_WHookClearKeyState();
}

static void test_count_sprites_uses_global_editor_bank(void)
{
    EffectData ignored[4];

    memset(ignored, 0xff, sizeof(ignored));
    memset(ed, 0, sizeof(ed));
    ed[0].bank = 1;
    ed[2].bank = 4;
    assert(ferret_CountSprites(ignored, 4) == 2);
}

static void test_particle_bank_bounds(void)
{
    int32_t cpad[2] = {0};

    memset(ed, 0, sizeof(ed));
    ed[0].bank = 2;
    ed[0].type = 0x40;
    ed[0].acc.vx = 7;
    ed[0].acc.vy = 0xf;

    press_key(VK_UP_TEST);
    ferret_ParticleBank(cpad, 0);
    assert(ed[0].type == 0x40);
    assert(ed[0].acc.vx == 0);

    press_key(VK_DOWN_TEST);
    ferret_ParticleBank(cpad, 0);
    assert(ed[0].type == 0x3f);

    press_key(VK_RIGHT_TEST);
    ferret_ParticleBank(cpad, 0);
    press_key(VK_UP_TEST);
    ferret_ParticleBank(cpad, 0);
    assert(ed[0].acc.vx == 0);
    release_keys();
}

static void test_embedded_bank_effect_limit(void)
{
    int32_t cpad[2] = {0};
    VECTOR location = {0};

    memset(ed, 0, sizeof(ed));
    gMaxEffect = 12;
    ed[0].bank = 3;
    ed[0].type = 12;

    press_key(VK_UP_TEST);
    ferret_EmmbeddedBank(cpad, &location, 0);
    assert(ed[0].type == 12);

    press_key(VK_DOWN_TEST);
    ferret_EmmbeddedBank(cpad, &location, 0);
    assert(ed[0].type == 11);
    release_keys();
}

static void test_ring_editor_wraps_canonical_fields(void)
{
    uint32_t cpad[2] = {0};
    RingData ring;

    memset(&ring, 0, sizeof(ring));
    ring.type = 0x32;
    press_key(VK_UP_TEST);
    ferret_EditRing(cpad, &ring);
    assert(ring.type == 0);

    press_key(VK_DOWN_TEST);
    ferret_EditRing(cpad, &ring);
    assert(ring.type == 0x32);

    press_key(VK_RIGHT_TEST);
    ferret_EditRing(cpad, &ring);
    ring.b.init = 0xff;
    press_key(VK_UP_TEST);
    ferret_EditRing(cpad, &ring);
    assert(ring.b.init == 0);
    release_keys();
}

static void test_sprite_bank_position_clamp(void)
{
    int32_t cpad[2] = {0};
    VECTOR location = {0};

    memset(ed, 0, sizeof(ed));
    ed[0].bank = 1;
    ed[0].pos.vx = 0x100;
    press_key(VK_UP_TEST);
    ferret_SpriteBank(cpad, &location, 0);
    assert(ed[0].pos.vx == 0x100);

    press_key(VK_DOWN_TEST);
    ferret_SpriteBank(cpad, &location, 0);
    assert(ed[0].pos.vx == 0xf8);

    press_key(VK_NEXT_TEST);
    ferret_SpriteBank(cpad, &location, 0);
    assert(ed[0].bright.limit == 0);
    release_keys();
}

static void test_particle_editor_fixed_point_step(void)
{
    int32_t cpad[2] = {0, 0x2000};
    VECTOR location = {0};
    Emiter *emit = &((Emiter *)aEmiter)[1];

    memset(emit, 0, sizeof(*emit));
    emit->rate = 0x20;
    emit->count = 1;
    emit->colorSpeed = 0x20;
    emit->deathSpeed = 0x20;
    ferret_ParticleEditor(cpad, &location);
    assert(emit->rate == 0xa0);
}

static void test_projectile_effect_lower_bound(void)
{
    uint32_t cpad[2] = {0};
    ProjType *projectiles = (ProjType *)maProjTypes;
    int index;

    memset(projectiles, 0, JPB_PROJECT_TYPE_BYTES);
    gMaxEffect = 12;
    for (index = 0; index < 4; ++index) {
        press_key(VK_RIGHT_TEST);
        ferret_EditProjectile(cpad);
    }
    projectiles[0].muzzelEffect = -5;
    release_keys();
    ferret_EditProjectile(cpad);
    assert(projectiles[0].muzzelEffect == -1);
}

static void test_zero_length_show_effect(void)
{
    VECTOR location = {0};

    assert(ferret_ShowEffect(ed, &location, 0) == 0);
}

int main(void)
{
    test_count_sprites_uses_global_editor_bank();
    test_particle_bank_bounds();
    test_embedded_bank_effect_limit();
    test_ring_editor_wraps_canonical_fields();
    test_sprite_bank_position_clamp();
    test_particle_editor_fixed_point_step();
    test_projectile_effect_lower_bound();
    test_zero_length_show_effect();
    puts("ferret tests passed");
    return 0;
}
