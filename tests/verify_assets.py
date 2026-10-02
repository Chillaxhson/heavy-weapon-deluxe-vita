import os
import xml.etree.ElementTree as ET

GAME_DIR = 'Heavy Weapon Deluxe'

def test_craft():
    tree = ET.parse(os.path.join(GAME_DIR, 'data/craft.xml'))
    root = tree.getroot()
    crafts = root.findall('Craft')
    print(f"[TEST] Found {len(crafts)} craft definitions in craft.xml")
    assert len(crafts) >= 15, "Expected at least 15 craft types"
    
    missing_textures = []
    for c in crafts:
        name = c.get('name')
        points = int(c.get('points', 0))
        armor = int(c.get('armor', 0))
        assert name, "Craft missing name"
        assert armor > 0, f"Craft {name} invalid armor: {armor}"
        
        # Check texture existence
        tex_path = os.path.join(GAME_DIR, f'Images/{name.lower()}.png')
        if not os.path.exists(tex_path):
            missing_textures.append(name)
            
    print(f"[TEST] Craft definitions valid. (Missing direct sprite files: {missing_textures})")
    return [c.get('name') for c in crafts]

def test_levels():
    tree = ET.parse(os.path.join(GAME_DIR, 'data/levels.xml'))
    root = tree.getroot()
    levels = root.findall('Level')
    print(f"[TEST] Found {len(levels)} levels in levels.xml")
    assert len(levels) == 19, f"Expected 19 levels, got {len(levels)}"
    
    for idx, lvl in enumerate(levels):
        name = lvl.get('name')
        length = int(lvl.get('length', 0))
        assert name, f"Level {idx} missing name"
        assert length > 0, f"Level {name} invalid length {length}"
        
        intel = lvl.findall('Intel')
        assert len(intel) > 0, f"Level {name} missing intel"
        
        # Check background theme textures
        theme = name.lower()
        sky_jpg = os.path.join(GAME_DIR, f'Images/Backgrounds/{theme}_sky.jpg')
        sky_png = os.path.join(GAME_DIR, f'Images/Backgrounds/{theme}_sky.png')
        assert os.path.exists(sky_jpg) or os.path.exists(sky_png), f"Missing sky for {theme}"
        
    print("[TEST] All 19 levels and background themes verified successfully!")
    return levels

def test_waves(valid_crafts):
    tree = ET.parse(os.path.join(GAME_DIR, 'data/waves.xml'))
    root = tree.getroot()
    levels = root.findall('Level')
    print(f"[TEST] Found {len(levels)} level wave schedules in waves.xml")
    assert len(levels) == 19, f"Expected 19 wave level entries, got {len(levels)}"
    
    total_waves = 0
    for l_idx, lvl in enumerate(levels):
        waves = lvl.findall('Wave')
        assert len(waves) > 0, f"Level {l_idx} has no waves"
        total_waves += len(waves)
        for w in waves:
            craft_entries = w.findall('Craft')
            assert len(craft_entries) > 0, "Wave has no craft"
            for ce in craft_entries:
                cid = ce.get('id')
                qty = int(ce.get('qty', 1))
                assert cid in valid_crafts, f"Wave references unknown craft ID: {cid}"
                assert qty > 0, f"Invalid quantity {qty}"
                
    print(f"[TEST] Verified {total_waves} waves across all 19 missions successfully!")

def test_bosses():
    tree = ET.parse(os.path.join(GAME_DIR, 'data/bosses.xml'))
    root = tree.getroot()
    bosses = list(root)
    print(f"[TEST] Found {len(bosses)} boss definitions in bosses.xml")
    assert len(bosses) >= 5, "Expected at least 5 boss types"
    for b in bosses:
        info = b.get('info')
        lvl1 = b.find('Level1')
        assert lvl1 is not None, f"Boss {b.tag} missing Level1"
        assert int(lvl1.get('armor', 0)) > 0, f"Boss {b.tag} invalid armor"
    print("[TEST] Boss structures verified successfully!")

def test_anims():
    tree = ET.parse(os.path.join(GAME_DIR, 'Images/Anims/Anims.xml'))
    root = tree.getroot()
    levels = root.findall('Level')
    print(f"[TEST] Found {len(levels)} level animation sets in Anims.xml")
    total_anims = 0
    missing_anim_pngs = []
    for l_idx, lvl in enumerate(levels):
        anims = lvl.findall('Anim')
        total_anims += len(anims)
        for a in anims:
            name = a.get('name')
            png = os.path.join(GAME_DIR, f'Images/Anims/{name}.png')
            if not os.path.exists(png):
                missing_anim_pngs.append(name)
    print(f"[TEST] Verified {total_anims} ambient animations. Missing files: {len(missing_anim_pngs)}")
    assert len(missing_anim_pngs) == 0, f"Missing animation strips: {missing_anim_pngs}"

def test_fonts():
    fonts = ['Normal', 'Computer', 'Keypunch16', 'Outline', 'RubberStampLET20', 'RubberStampLET42', 'StationFont']
    for f in fonts:
        txt = os.path.join(GAME_DIR, f'Fonts/{f}.txt')
        png1 = os.path.join(GAME_DIR, f'Fonts/_{f}.png')
        png2 = os.path.join(GAME_DIR, f'Fonts/{f}.png')
        assert os.path.exists(txt), f"Missing font descriptor: {txt}"
        assert os.path.exists(png1) or os.path.exists(png2), f"Missing font texture: {f}"
    print(f"[TEST] Verified all {len(fonts)} PopCap ImageFonts!")

if __name__ == '__main__':
    crafts = test_craft()
    test_levels()
    test_waves(crafts)
    test_bosses()
    test_anims()
    test_fonts()
    print("\n[ALL ASSET AND SCHEMA TESTS PASSED 100%]")
