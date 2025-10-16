# P1-PERF-002: New Repository Push Guide

**Date**: 2025-10-16  
**Status**: READY FOR EXECUTION

---

## Overview

This guide provides step-by-step instructions to create a new GitHub repository and push P1-PERF-002 feature branch.

---

## Step 1: Create New Repository on GitHub

### Option A: Using GitHub Web Interface

1. Go to https://github.com/new
2. **Repository name**: `keycuda-p1-perf-002`
3. **Description**: "P1-PERF-002: Dual-Stream Pipeline + Adaptive Batch Sizing - CUDA Performance Optimization"
4. **Visibility**: Public (or Private, same as original)
5. **Initialize**: Do NOT initialize with README
6. Click "Create repository"

### Option B: Using GitHub CLI

```bash
gh repo create keycuda-p1-perf-002 \
  --description "P1-PERF-002: Dual-Stream Pipeline + Adaptive Batch Sizing" \
  --public \
  --source=. \
  --remote=origin \
  --push
```

---

## Step 2: Clone New Repository

```bash
# Create a new directory for the new repo
mkdir -p ~/github-repos
cd ~/github-repos

# Clone the new repository
git clone https://github.com/pest88-spec/keycuda-p1-perf-002.git
cd keycuda-p1-perf-002
```

---

## Step 3: Add Old Repository as Remote

```bash
# Add old repository as remote
git remote add old-repo /mnt/d/mybitcoin/puzzlekeyhunt/PuzzleKeyhunt

# Verify remotes
git remote -v
# Should show:
# origin  https://github.com/pest88-spec/keycuda-p1-perf-002.git (fetch)
# origin  https://github.com/pest88-spec/keycuda-p1-perf-002.git (push)
# old-repo /mnt/d/mybitcoin/puzzlekeyhunt/PuzzleKeyhunt (fetch)
# old-repo /mnt/d/mybitcoin/puzzlekeyhunt/PuzzleKeyhunt (push)
```

---

## Step 4: Fetch Feature Branch from Old Repository

```bash
# Fetch the feature branch
git fetch old-repo feature/p1-perf-002

# Verify it was fetched
git branch -a
# Should show:
# * main
#   remotes/old-repo/feature/p1-perf-002
```

---

## Step 5: Create Local Feature Branch

```bash
# Create local feature branch from fetched branch
git checkout -b feature/p1-perf-002 old-repo/feature/p1-perf-002

# Verify branch
git branch
# Should show:
# * feature/p1-perf-002
#   main
```

---

## Step 6: Push Feature Branch to New Repository

```bash
# Push feature branch to new repository
git push -u origin feature/p1-perf-002

# Verify push
git branch -v
# Should show:
# * feature/p1-perf-002 925f441 [origin/feature/p1-perf-002] docs(P1-PERF-002): Add comprehensive documentation
#   main              <hash> [origin/main] <message>
```

---

## Step 7: Verify on GitHub

1. Go to https://github.com/pest88-spec/keycuda-p1-perf-002
2. Click "Branches"
3. Verify `feature/p1-perf-002` branch is visible
4. Click on branch to view commits
5. Verify 3 commits are present:
   - feat(P1-PERF-002): Implement dual-stream pipeline...
   - feat(P1-PERF-002): Add production deployment automation...
   - docs(P1-PERF-002): Add comprehensive documentation...

---

## Step 8: Create Pull Request (Optional)

### If You Want to Merge to Main

1. Go to https://github.com/pest88-spec/keycuda-p1-perf-002
2. Click "Pull requests"
3. Click "New pull request"
4. **Base**: main
5. **Compare**: feature/p1-perf-002
6. Click "Create pull request"
7. Add description and review notes
8. Click "Create pull request"

### To Merge PR

1. Click "Merge pull request"
2. Choose merge strategy (Squash, Rebase, or Merge)
3. Click "Confirm merge"

---

## Complete Commands (Quick Reference)

```bash
# 1. Create new repo on GitHub (https://github.com/new)
#    Name: keycuda-p1-perf-002
#    Do NOT initialize

# 2. Clone new repo
mkdir -p ~/github-repos
cd ~/github-repos
git clone https://github.com/pest88-spec/keycuda-p1-perf-002.git
cd keycuda-p1-perf-002

# 3. Add old repo as remote
git remote add old-repo /mnt/d/mybitcoin/puzzlekeyhunt/PuzzleKeyhunt

# 4. Fetch feature branch
git fetch old-repo feature/p1-perf-002

# 5. Create local feature branch
git checkout -b feature/p1-perf-002 old-repo/feature/p1-perf-002

# 6. Push to new repo
git push -u origin feature/p1-perf-002

# 7. Verify on GitHub
# Go to https://github.com/pest88-spec/keycuda-p1-perf-002/branches
```

---

## Verification Checklist

- [ ] New repository created on GitHub
- [ ] Repository name: `keycuda-p1-perf-002`
- [ ] Feature branch fetched from old repository
- [ ] Feature branch pushed to new repository
- [ ] 3 commits visible on GitHub
- [ ] Commit messages correct
- [ ] All files present (scripts, documentation)

---

## Next Steps After Push

### Option 1: Deploy from New Repository

```bash
# Clone new repository
git clone https://github.com/pest88-spec/keycuda-p1-perf-002.git
cd keycuda-p1-perf-002
git checkout feature/p1-perf-002

# Build and deploy
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..
bash scripts/deploy_prod.sh canary 300
```

### Option 2: Keep Using Local Repository

```bash
# Continue using local repository
cd /mnt/d/mybitcoin/puzzlekeyhunt/PuzzleKeyhunt
bash scripts/deploy_prod.sh canary 300
```

### Option 3: Merge to Main and Deploy

```bash
# Create PR on GitHub
# Merge PR
# Clone main branch
git clone https://github.com/pest88-spec/keycuda-p1-perf-002.git
cd keycuda-p1-perf-002
git checkout main

# Build and deploy
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..
bash scripts/deploy_prod.sh canary 300
```

---

## Troubleshooting

### Error: "Repository not found"

**Cause**: Repository not created yet or wrong URL

**Solution**: 
1. Go to https://github.com/new
2. Create repository with name `keycuda-p1-perf-002`
3. Retry clone

### Error: "Permission denied"

**Cause**: SSH key not configured

**Solution**:
```bash
# Check SSH key
ls -la ~/.ssh/id_ed25519_github

# Test SSH connection
ssh -T git@github.com

# If not working, add SSH key to ssh-agent
ssh-add ~/.ssh/id_ed25519_github
```

### Error: "fatal: 'old-repo' does not appear to be a 'git' repository"

**Cause**: Wrong path to old repository

**Solution**:
```bash
# Verify old repository path
ls -la /mnt/d/mybitcoin/puzzlekeyhunt/PuzzleKeyhunt/.git

# Update remote if needed
git remote remove old-repo
git remote add old-repo /correct/path/to/PuzzleKeyhunt
```

---

## Timeline

### Immediate (Now)

- [ ] Create new repository on GitHub
- [ ] Clone new repository
- [ ] Push feature branch
- [ ] Verify on GitHub

### Short-term (1-2 hours)

- [ ] Deploy P1-PERF-002 from new repository
- [ ] Monitor performance metrics

### Medium-term (1 week)

- [ ] Merge to main branch
- [ ] Create release tag
- [ ] Publish release notes

---

## Conclusion

**This approach**:
- ✅ Avoids corrupted repository
- ✅ Creates clean new repository
- ✅ Maintains full commit history
- ✅ Allows for proper version control
- ✅ Enables collaboration

**Next**: Execute these steps to push P1-PERF-002 to new repository.

---

**Guide Status**: ✅ COMPLETE  
**Ready to Execute**: ✅ YES  
**Last Updated**: 2025-10-16

