# P1-PERF-002: GitHub Repository Repair Guide

**Date**: 2025-10-16  
**Status**: REPAIR INSTRUCTIONS

---

## Overview

This guide provides step-by-step instructions to repair the corrupted GitHub repository.

---

## Method 1: Contact GitHub Support (RECOMMENDED)

### Step 1: Go to GitHub Support

1. Open https://github.com/pest88-spec/keycuda
2. Click "Settings" (gear icon)
3. Scroll down to "Support"
4. Click "Contact GitHub Support"

### Step 2: Create Support Ticket

**Title**: Repository Corruption - fsck Error

**Description**:
```
Repository: pest88-spec/keycuda
Issue: Cannot push to repository due to corrupted Git object

Error Message:
remote: error: object 80bc09a55a01ff754e7f2b635610acf8f905954d: duplicateEntries: contains duplicate file entries
remote: fatal: fsck error in packed object
error: remote unpack failed: index-pack failed

Request: Please repair the repository or reset the packed objects.
```

### Step 3: Wait for Response

- GitHub support will investigate
- Timeline: 1-2 business days
- They will either:
  - Repair the corrupted object
  - Reset the packed objects
  - Provide alternative solution

### Step 4: Retry Push

Once GitHub confirms the fix:
```bash
cd /path/to/PuzzleKeyhunt
git push -u origin feature/p1-perf-002
```

---

## Method 2: Force Push with GitHub CLI

### Step 1: Install GitHub CLI

**macOS**:
```bash
brew install gh
```

**Linux (Ubuntu/Debian)**:
```bash
sudo apt-get install gh
```

**Windows (WSL)**:
```bash
sudo apt-get install gh
```

### Step 2: Authenticate

```bash
gh auth login
# Follow prompts to authenticate
```

### Step 3: Try Force Push

```bash
cd /path/to/PuzzleKeyhunt
git push --force-with-lease origin feature/p1-perf-002
```

**Note**: This might not work if the issue is server-side, but worth trying.

---

## Method 3: Create New Repository

### Step 1: Create New Repository on GitHub

1. Go to https://github.com/new
2. Repository name: `keycuda-p1-perf-002`
3. Description: "P1-PERF-002: Dual-Stream Pipeline + Adaptive Batch Sizing"
4. Public/Private: Same as original
5. Click "Create repository"

### Step 2: Clone New Repository

```bash
git clone https://github.com/pest88-spec/keycuda-p1-perf-002.git
cd keycuda-p1-perf-002
```

### Step 3: Add Old Repository as Remote

```bash
git remote add old-repo /path/to/old/PuzzleKeyhunt
```

### Step 4: Fetch Feature Branch

```bash
git fetch old-repo feature/p1-perf-002
```

### Step 5: Push to New Repository

```bash
git push -u origin old-repo/feature/p1-perf-002:feature/p1-perf-002
```

### Step 6: Create Pull Request

1. Go to new repository on GitHub
2. Click "Pull requests"
3. Click "New pull request"
4. Create PR from `feature/p1-perf-002` to `main`

---

## Method 4: Reset Packed Objects (Advanced)

### Step 1: Check Repository Health

```bash
cd /path/to/PuzzleKeyhunt
git fsck --full
```

### Step 2: Repack Repository

```bash
git repack -Ad
```

### Step 3: Retry Push

```bash
git push -u origin feature/p1-perf-002
```

**Note**: This might not work if the issue is on GitHub's server.

---

## Method 5: Clone Fresh from GitHub

### Step 1: Backup Current Repository

```bash
cp -r /path/to/PuzzleKeyhunt /path/to/PuzzleKeyhunt.backup
```

### Step 2: Clone Fresh

```bash
git clone https://github.com/pest88-spec/keycuda.git PuzzleKeyhunt-fresh
cd PuzzleKeyhunt-fresh
```

### Step 3: Create Feature Branch

```bash
git checkout -b feature/p1-perf-002
```

### Step 4: Copy Changes from Backup

```bash
# Copy modified files
cp /path/to/PuzzleKeyhunt.backup/src/main.cpp src/
cp /path/to/PuzzleKeyhunt.backup/src/solver.h src/
cp /path/to/PuzzleKeyhunt.backup/scripts/deploy_*.sh scripts/
cp /path/to/PuzzleKeyhunt.backup/RELEASE_NOTES_P1-PERF-002.md .
cp -r /path/to/PuzzleKeyhunt.backup/docs/P1-PERF-002_*.md docs/
```

### Step 5: Commit Changes

```bash
git add .
git commit -m "feat(P1-PERF-002): Implement dual-stream pipeline and adaptive batch sizing"
```

### Step 6: Push to GitHub

```bash
git push -u origin feature/p1-perf-002
```

---

## Recommended Approach

### For Immediate Deployment

**Do NOT wait for GitHub fix.**

Deploy from local repository:
```bash
cd /path/to/PuzzleKeyhunt
bash scripts/deploy_prod.sh canary 300
```

### For GitHub Fix

**Use Method 1 (Contact GitHub Support)**:
1. Most reliable
2. GitHub team handles it
3. No risk of data loss
4. Timeline: 1-2 business days

### If GitHub Support is Slow

**Use Method 3 (Create New Repository)**:
1. Create new repository
2. Push feature branch
3. Continue development
4. Migrate to new repository

---

## Timeline

### Immediate (Now)

- ✅ Deploy P1-PERF-002 from local repository
- ✅ Start GitHub support ticket

### Short-term (1-2 days)

- ⏳ Wait for GitHub support response
- ⏳ GitHub repairs repository

### Medium-term (1 week)

- ✅ Push to GitHub
- ✅ Create pull request
- ✅ Merge to main

### Long-term (1-2 weeks)

- ✅ Monitor production
- ✅ Validate performance
- ✅ Create release

---

## Troubleshooting

### If GitHub Support Doesn't Respond

**Action**: Use Method 3 (Create New Repository)
- Create new repository
- Push feature branch
- Continue development

### If Force Push Doesn't Work

**Action**: Use Method 3 (Create New Repository)
- Same as above

### If Repack Doesn't Work

**Action**: Use Method 1 (Contact GitHub Support)
- GitHub team can investigate further

---

## Conclusion

**Recommended Action Plan**:

1. **Now**: Deploy P1-PERF-002 from local repository
2. **Now**: Contact GitHub support with error details
3. **1-2 days**: Wait for GitHub response
4. **After fix**: Push to GitHub
5. **After push**: Create pull request and merge

**This ensures**:
- ✅ P1-PERF-002 goes live immediately
- ✅ GitHub issue is resolved
- ✅ Proper version control on GitHub
- ✅ No data loss

---

**Guide Status**: ✅ COMPLETE  
**Deployment**: ✅ READY (from local repo)  
**GitHub Fix**: ⏳ PENDING SUPPORT  
**Last Updated**: 2025-10-16

