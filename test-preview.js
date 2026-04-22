// Simple test to verify LaTeX preview functionality

// Test the LaTeX rendering logic
function testLatexRendering() {
  console.log('Testing LaTeX preview functionality...');

  // Sample LaTeX content
  const latexContent = `
\\documentclass{article}
\\begin{document}
\\section{Introduction}
This is a test with math: $E = mc^2$

Block math: $$\\int_0^\\infty e^{-x^2} dx$$

Bold text: \\textbf{Hello World}
\\end{document}
`;

  console.log('Sample content length:', latexContent.length);
  console.log('Content preview:', latexContent.substring(0, 100) + '...');

  // Test regex patterns used in LatexPreview
  const inlineMathMatches = latexContent.match(/\$([^$\n]+?)\$/g);
  const blockMathMatches = latexContent.match(/\$\$([^$]+?)\$\$/g);
  const sectionMatches = latexContent.match(/\\section\*?\{([^}]+)\}/g);
  const boldMatches = latexContent.match(/\\textbf\{([^}]+)\}/g);

  console.log('Inline math matches:', inlineMathMatches);
  console.log('Block math matches:', blockMathMatches);
  console.log('Section matches:', sectionMatches);
  console.log('Bold matches:', boldMatches);

  return {
    hasContent: latexContent.trim().length > 0,
    hasMath: inlineMathMatches || blockMathMatches,
    hasStructure: sectionMatches || boldMatches
  };
}

const result = testLatexRendering();
console.log('Test result:', result);