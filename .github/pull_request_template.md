<!-- See CONTRIBUTING.md. -->

**What this does**


**Checklist**
- [ ] `ninja` passes, with both Black 2 and White 2 matching their SHA1s
- [ ] Configs changed only through the scripts, in both versions
- [ ] Functions that don't match have a row in `docs/nonmatching-functions.md`
- [ ] Only the files I changed are formatted, with `clang-format` 23.1.1
- [ ] `tools/decomp/check_style.py --diff origin/main` reports nothing, or each finding has a `// style-ok:` reason
- [ ] No workaround kept for a match ([Code standards](docs/code-standards.md)): no gotos to a return, dummy locals,
      hand-expanded macros or byte-offset casts
