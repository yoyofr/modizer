// Main application logic
document.addEventListener('DOMContentLoaded', () => {
    initializeLanguageSelector();
    initializeNavigation();
    initializeScrollToTop();
    loadPreferredLanguage();
});

// ========================================
// Language Selector
// ========================================
function initializeLanguageSelector() {
    const langButtons = document.querySelectorAll('.lang-btn');

    langButtons.forEach(button => {
        button.addEventListener('click', () => {
            const lang = button.getAttribute('data-lang');

            // Update active state
            langButtons.forEach(btn => btn.classList.remove('active'));
            button.classList.add('active');

            // Translate page
            translatePage(lang);
        });
    });
}

function loadPreferredLanguage() {
    // Check for saved preference
    const savedLang = localStorage.getItem('preferredLanguage');

    // Detect browser language as fallback
    const browserLang = navigator.language.split('-')[0];

    // Determine which language to use
    const lang = savedLang || (browserLang === 'fr' ? 'fr' : 'en');

    // Update UI
    const langButton = document.querySelector(`.lang-btn[data-lang="${lang}"]`);
    if (langButton) {
        langButton.click();
    }
}

// ========================================
// Navigation
// ========================================
function initializeNavigation() {
    const navLinks = document.querySelectorAll('.nav-link');
    const sections = document.querySelectorAll('.content-section');

    navLinks.forEach(link => {
        link.addEventListener('click', (e) => {
            e.preventDefault();

            const targetId = link.getAttribute('href').substring(1);

            // Update active nav link
            navLinks.forEach(l => l.classList.remove('active'));
            link.classList.add('active');

            // Show target section
            sections.forEach(section => {
                if (section.id === targetId) {
                    section.classList.add('active');
                } else {
                    section.classList.remove('active');
                }
            });

            // Scroll to top smoothly
            window.scrollTo({
                top: 0,
                behavior: 'smooth'
            });

            // Update URL hash without jumping
            history.pushState(null, null, `#${targetId}`);
        });
    });

    // Handle initial hash on page load
    handleInitialHash();

    // Handle browser back/forward buttons
    window.addEventListener('popstate', handleInitialHash);
}

function handleInitialHash() {
    const hash = window.location.hash.substring(1);
    if (hash) {
        const targetLink = document.querySelector(`.nav-link[href="#${hash}"]`);
        if (targetLink) {
            targetLink.click();
        }
    }
}

// ========================================
// Scroll to Top on Section Change
// ========================================
function initializeScrollToTop() {
    // Optional: Add a scroll-to-top button for long pages
    let scrollButton = document.createElement('button');
    scrollButton.innerHTML = '↑';
    scrollButton.className = 'scroll-to-top';
    scrollButton.style.cssText = `
        position: fixed;
        bottom: 30px;
        right: 30px;
        width: 50px;
        height: 50px;
        border-radius: 50%;
        background: linear-gradient(135deg, #6366f1, #ec4899);
        color: white;
        border: none;
        font-size: 24px;
        cursor: pointer;
        box-shadow: 0 4px 12px rgba(99, 102, 241, 0.4);
        opacity: 0;
        visibility: hidden;
        transition: all 0.3s;
        z-index: 999;
    `;

    document.body.appendChild(scrollButton);

    // Show/hide button based on scroll position
    window.addEventListener('scroll', () => {
        if (window.pageYOffset > 300) {
            scrollButton.style.opacity = '1';
            scrollButton.style.visibility = 'visible';
        } else {
            scrollButton.style.opacity = '0';
            scrollButton.style.visibility = 'hidden';
        }
    });

    // Scroll to top on click
    scrollButton.addEventListener('click', () => {
        window.scrollTo({
            top: 0,
            behavior: 'smooth'
        });
    });
}

// ========================================
// FAQ Accordion (Optional Enhancement)
// ========================================
function initializeFAQAccordion() {
    const faqItems = document.querySelectorAll('.faq-item');

    faqItems.forEach(item => {
        const question = item.querySelector('.faq-question');
        const answer = item.querySelector('.faq-answer');

        // Initially hide answers (optional)
        // answer.style.display = 'none';

        question.addEventListener('click', () => {
            // Toggle answer visibility
            const isVisible = answer.style.display !== 'none';
            answer.style.display = isVisible ? 'none' : 'block';

            // Add rotation to icon if you want
            item.classList.toggle('expanded');
        });
    });
}

// ========================================
// Dark Mode Toggle (Optional)
// ========================================
function initializeDarkMode() {
    // Check for saved preference or system preference
    const savedTheme = localStorage.getItem('theme');
    const systemPrefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;

    if (savedTheme === 'dark' || (!savedTheme && systemPrefersDark)) {
        document.body.classList.add('dark-mode');
    }

    // Create toggle button (you can add this to your HTML if needed)
    // Implementation left as an exercise
}

// ========================================
// Search Functionality (Optional)
// ========================================
function initializeSearch() {
    // Add a search input to your HTML
    // Filter content based on search query
    // Implementation left as an exercise
}

// ========================================
// Analytics (Optional)
// ========================================
function trackPageView(section) {
    // Add your analytics tracking here
    // Example: gtag('event', 'page_view', { page_path: section });
    console.log('Section viewed:', section);
}

// Export functions for use in embedded environments (iOS WebView)
if (typeof window !== 'undefined') {
    window.ModizerGuide = {
        setLanguage: (lang) => {
            const button = document.querySelector(`.lang-btn[data-lang="${lang}"]`);
            if (button) button.click();
        },
        navigateToSection: (sectionId) => {
            const link = document.querySelector(`.nav-link[href="#${sectionId}"]`);
            if (link) link.click();
        },
        getCurrentSection: () => {
            const activeSection = document.querySelector('.content-section.active');
            return activeSection ? activeSection.id : null;
        }
    };
}
