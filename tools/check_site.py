#!/usr/bin/env python3
from __future__ import annotations

from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
SITE = ROOT / "site"


class References(HTMLParser):
    def __init__(self) -> None:
        super().__init__()
        self.references: list[tuple[str, str]] = []
        self.images_without_alt: list[str] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        values = dict(attrs)
        if tag == "a" and values.get("href"):
            self.references.append((tag, values["href"] or ""))
        if tag == "img":
            if values.get("src"):
                self.references.append((tag, values["src"] or ""))
            if not (values.get("alt") or "").strip():
                self.images_without_alt.append(values.get("src") or "<unknown>")


def target_path(page: Path, reference: str) -> Path | None:
    parsed = urlsplit(reference)
    if parsed.scheme or parsed.netloc or reference.startswith(("mailto:", "tel:")):
        return None
    raw = unquote(parsed.path)
    if not raw:
        return page
    site_prefix = f"/{ROOT.name}/"
    if raw == site_prefix[:-1]:
        raw = "/"
    elif raw.startswith(site_prefix):
        raw = "/" + raw[len(site_prefix):]
    target = SITE / raw.lstrip("/") if raw.startswith("/") else page.parent / raw
    if raw.endswith("/"):
        target /= "index.html"
    return target.resolve()


def main() -> None:
    errors: list[str] = []
    pages = sorted(SITE.rglob("*.html"))
    if not pages:
        raise SystemExit("site check failed: build output is empty")
    for page in pages:
        parser = References()
        parser.feed(page.read_text(encoding="utf-8"))
        for image in parser.images_without_alt:
            errors.append(f"{page.relative_to(SITE)}: image lacks alt text: {image}")
        for tag, reference in parser.references:
            target = target_path(page, reference)
            if target is None:
                continue
            if not target.exists():
                errors.append(
                    f"{page.relative_to(SITE)}: broken {tag} reference {reference}"
                )
    if errors:
        raise SystemExit("site check failed:\n- " + "\n- ".join(errors))
    print(f"site links and image alternatives verified across {len(pages)} HTML pages")


if __name__ == "__main__":
    main()
