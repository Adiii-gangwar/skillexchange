/* ===================================================================
   SkillSwap - Mock Data Store
   Used for frontend client-side rendering & PBL demo presentation.
   Note: All interactions are managed in the browser (client-side).
   =================================================================== */

const MOCK_CURRENT_USER = {
  name: "Alex Morgan",
  email: "alex.morgan@college.edu",
  rollNo: "CS23B1042",
  department: "Computer Science & Eng.",
  year: "3rd Year (Semester 6)",
  rating: 4.9,
  completedSwaps: 12,
  avatarInitial: "AM",
  bio: "Passionate about full-stack web development and Python scripting. Looking forward to learning UI/UX design and cloud deployment from peers!",
  skillsTeach: [
    { name: "Python Programming", level: "Advanced" },
    { name: "JavaScript & DOM", level: "Advanced" },
    { name: "SQL & Relational DBs", level: "Intermediate" },
    { name: "Git & GitHub Workflow", level: "Intermediate" }
  ],
  skillsLearn: [
    { name: "Figma & UI/UX Design", goal: "Build portfolio mockups" },
    { name: "Docker & Containerization", goal: "Cloud deployment project" },
    { name: "Machine Learning Basics", goal: "Semester elective preparation" }
  ]
};

const MOCK_CATEGORIES = [
  { id: "all", label: "All Skills", icon: "fa-layer-group" },
  { id: "web-dev", label: "Web Development", icon: "fa-code" },
  { id: "ai-ml", label: "AI & Machine Learning", icon: "fa-brain" },
  { id: "design", label: "UI/UX & Design", icon: "fa-pen-nib" },
  { id: "dsa", label: "DSA & Problem Solving", icon: "fa-terminal" },
  { id: "mobile-dev", label: "Mobile Apps", icon: "fa-mobile-screen" },
  { id: "cloud", label: "Cloud & DevOps", icon: "fa-cloud" },
  { id: "soft-skills", label: "Public Speaking & Soft Skills", icon: "fa-comments" }
];

const MOCK_STUDENTS = [
  {
    id: 1,
    name: "Aarav Sharma",
    avatarInitial: "AS",
    department: "Computer Science",
    year: "3rd Year",
    category: "web-dev",
    rating: 4.9,
    swapsCount: 14,
    bio: "Full stack enthusiast. Built multiple projects using Node.js & React. Keen to learn Competitive Programming and advanced graph algorithms.",
    teach: [
      { name: "Node.js & Express", level: "Advanced" },
      { name: "REST APIs", level: "Advanced" },
      { name: "MongoDB", level: "Intermediate" }
    ],
    learn: [
      { name: "DSA (Trees & Graphs)" },
      { name: "C++ STL" }
    ],
    availability: "Weekdays (Evenings)"
  },
  {
    id: 2,
    name: "Sneha Patel",
    avatarInitial: "SP",
    department: "Information Technology",
    year: "2nd Year",
    category: "design",
    rating: 5.0,
    swapsCount: 9,
    bio: "Self-taught product designer with expertise in Figma, design systems, and wireframing. Looking for someone to guide me in frontend HTML/CSS integration.",
    teach: [
      { name: "Figma & Wireframing", level: "Advanced" },
      { name: "Color Theory & Typography", level: "Advanced" },
      { name: "User Research", level: "Intermediate" }
    ],
    learn: [
      { name: "HTML5 & Modern CSS" },
      { name: "Responsive Layouts" }
    ],
    availability: "Weekends & Tuesdays"
  },
  {
    id: 3,
    name: "Rohan Verma",
    avatarInitial: "RV",
    department: "Electronics & Comm.",
    year: "4th Year",
    category: "ai-ml",
    rating: 4.8,
    swapsCount: 18,
    bio: "Final year student working on computer vision and neural networks. Happy to teach Python for data science in exchange for IoT / Embedded C.",
    teach: [
      { name: "Python for Data Science", level: "Advanced" },
      { name: "TensorFlow & PyTorch", level: "Intermediate" },
      { name: "OpenCV", level: "Intermediate" }
    ],
    learn: [
      { name: "Embedded C" },
      { name: "Arduino / ESP32" }
    ],
    availability: "Flexible (Library Sessions)"
  },
  {
    id: 4,
    name: "Ananya Iyer",
    avatarInitial: "AI",
    department: "Computer Science",
    year: "3rd Year",
    category: "dsa",
    rating: 4.9,
    swapsCount: 22,
    bio: "Candidate Master on Codeforces, 500+ LeetCode problems solved. I can help you conquer coding interviews! Want to learn Flutter or React Native.",
    teach: [
      { name: "Data Structures & Algos", level: "Advanced" },
      { name: "Dynamic Programming", level: "Advanced" },
      { name: "C++ Optimization", level: "Advanced" }
    ],
    learn: [
      { name: "Flutter App Dev" },
      { name: "State Management" }
    ],
    availability: "Mon, Wed, Fri (After 5 PM)"
  },
  {
    id: 5,
    name: "Vikramaditya Singh",
    avatarInitial: "VS",
    department: "Mechanical Eng.",
    year: "3rd Year",
    category: "cloud",
    rating: 4.7,
    swapsCount: 7,
    bio: "Pivoted towards DevOps & Cloud infrastructure. AWS Certified Cloud Practitioner. Looking for guidance on Linux shell scripting & Docker.",
    teach: [
      { name: "AWS Cloud Fundamentals", level: "Intermediate" },
      { name: "CI/CD Concepts", level: "Intermediate" },
      { name: "CAD / SolidWorks", level: "Advanced" }
    ],
    learn: [
      { name: "Linux Bash Scripting" },
      { name: "Kubernetes Basics" }
    ],
    availability: "Saturdays & Sundays"
  },
  {
    id: 6,
    name: "Pooja Hegde",
    avatarInitial: "PH",
    department: "Information Technology",
    year: "2nd Year",
    category: "mobile-dev",
    rating: 4.9,
    swapsCount: 11,
    bio: "Building mobile applications with Flutter and Firebase. Love clean architecture! Want to improve my public speaking and interview presentation skills.",
    teach: [
      { name: "Flutter Cross-Platform", level: "Intermediate" },
      { name: "Firebase Authentication", level: "Intermediate" }
    ],
    learn: [
      { name: "Public Speaking & GD" },
      { name: "Tech Interview Prep" }
    ],
    availability: "Thursday & Sunday"
  },
  {
    id: 7,
    name: "Kabir Mehta",
    avatarInitial: "KM",
    department: "Management & Tech",
    year: "4th Year",
    category: "soft-skills",
    rating: 5.0,
    swapsCount: 25,
    bio: "College Debating Society President. Helped 30+ students ace behavioral interviews, resume screening, and stage confidence. Want to learn Python for automation.",
    teach: [
      { name: "Public Speaking & Pitching", level: "Advanced" },
      { name: "Resume & LinkedIn Review", level: "Advanced" },
      { name: "Group Discussion Strategy", level: "Advanced" }
    ],
    learn: [
      { name: "Python Scripting" },
      { name: "Web Scraping" }
    ],
    availability: "Evenings (Online/Offline)"
  },
  {
    id: 8,
    name: "Meera Nambiar",
    avatarInitial: "MN",
    department: "Computer Science",
    year: "2nd Year",
    category: "web-dev",
    rating: 4.8,
    swapsCount: 8,
    bio: "Front-end enthusiast exploring CSS animations, Tailwind, and interactive widgets. Seeking a mentor in SQL databases and schema design.",
    teach: [
      { name: "Modern CSS & SASS", level: "Advanced" },
      { name: "Tailwind CSS", level: "Intermediate" },
      { name: "Vanilla JavaScript", level: "Intermediate" }
    ],
    learn: [
      { name: "PostgreSQL & Database Design" },
      { name: "Backend APIs" }
    ],
    availability: "Weekdays 4 PM - 7 PM"
  }
];

const MOCK_CONNECTIONS = {
  active: [
    {
      id: 101,
      studentName: "Sneha Patel",
      avatarInitial: "SP",
      department: "IT • 2nd Year",
      userTeaches: "Python & DOM Scripting",
      partnerTeaches: "Figma & UI/UX Design",
      status: "In Progress (3 sessions completed)",
      nextSession: "Tomorrow at 5:30 PM",
      rating: 5.0
    },
    {
      id: 102,
      studentName: "Ananya Iyer",
      avatarInitial: "AI",
      department: "CSE • 3rd Year",
      userTeaches: "Git & GitHub Team Workflows",
      partnerTeaches: "Binary Search & Tree Traversals",
      status: "In Progress (2 sessions completed)",
      nextSession: "Friday at 4:00 PM",
      rating: 4.9
    },
    {
      id: 103,
      studentName: "Vikramaditya Singh",
      avatarInitial: "VS",
      department: "Mech • 3rd Year",
      userTeaches: "SQL Query Basics",
      partnerTeaches: "AWS Cloud Fundamentals",
      status: "Scheduled first session",
      nextSession: "Sunday at 11:00 AM",
      rating: 4.7
    }
  ],
  pending: [
    {
      id: 201,
      studentName: "Rohan Verma",
      avatarInitial: "RV",
      department: "ECE • 4th Year",
      message: "Hey Alex! I saw your profile and noticed you have great JavaScript skills. Would love to swap for Python Machine Learning fundamentals!",
      userSkillRequested: "JavaScript & DOM",
      partnerSkillOffered: "Python for Data Science",
      date: "Received 2 hours ago"
    },
    {
      id: 202,
      studentName: "Kabir Mehta",
      avatarInitial: "KM",
      department: "M&T • 4th Year",
      message: "Hi Alex, I can help you polish your upcoming technical presentation & resume in exchange for your SQL database insights.",
      userSkillRequested: "SQL & Relational DBs",
      partnerSkillOffered: "Public Speaking & Pitching",
      date: "Received yesterday"
    }
  ],
  sent: [
    {
      id: 301,
      studentName: "Pooja Hegde",
      avatarInitial: "PH",
      department: "IT • 2nd Year",
      message: "Hey Pooja, I'm interested in building Flutter apps. Would love to teach you Python or Git in exchange!",
      skillOffered: "Git & GitHub Workflow",
      skillRequested: "Flutter Cross-Platform",
      date: "Sent 3 days ago",
      status: "Pending Partner Acceptance"
    }
  ]
};
